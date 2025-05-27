// local project
#include "concurrentqueue.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server_builder.h"
#include "operation_store.h"
#include "r_eval_service_impl.h"
#include "r_result.h"
#include "r_task.h"
#include "r_worker.h"
#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <absl/log/log.h>
#include <grpcpp/grpcpp.h>
#include <memory>

int main() {
  // namespace for ConcurrentQueue
  using namespace moodycamel;
  using namespace RWorker;

  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kFatal);

  LOG(INFO) << "haRness: main program starting up";

  // stop source + token for the rworker thread
  std::stop_source rworker_ssource;
  std::stop_token rworker_stoken = rworker_ssource.get_token();

  // global queues network <-> rworker
  ConcurrentQueue<std::unique_ptr<RTask>> taskQueue;
  ConcurrentQueue<std::unique_ptr<RResponse>> responseQueue;
  // store for grpc to keep track of operations
  EvalOperationStore operationStore;

  // create worker thread before creating gRPC server
  std::thread rWorkerThread(RWorker::r_worker_thread, rworker_stoken,
                            std::ref(taskQueue), std::ref(responseQueue));

  REvalServiceImpl rEvalService(std::ref(operationStore), std::ref(taskQueue),
                                std::ref(responseQueue));

  std::string server_address("0.0.0.0:50051");

  grpc::ServerBuilder serverBuilder;
  serverBuilder.AddListeningPort(server_address,
                                 grpc::InsecureServerCredentials());

  serverBuilder.RegisterService(&rEvalService);
  std::unique_ptr<grpc::Server> server(serverBuilder.BuildAndStart());

  LOG(INFO) << "gRPC Server Listening on " << server_address;

  // try to make custom sigint handler
  // TODO: handling the reference passing will be more complex.
  // we need to just have signal handler set some global variable
  // and start the server in another thread, so that the main func
  // can just block on the signal var and clean up here.

  server->Wait();

  // will block
  if (rWorkerThread.joinable()) {
    rWorkerThread.join();
  }

  return 0;
}
