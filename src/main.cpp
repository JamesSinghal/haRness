// local project
#include "concurrentqueue.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server_builder.h"
#include "operation_store.h"
#include "r_eval_service_impl.h"
#include "r_result.h"
#include "r_task.h"
#include "r_worker.h"
#include <memory>
#include <absl/log/log.h>
#include <grpcpp/grpcpp.h>

int main() {
  // namespace for ConcurrentQueue
  using namespace moodycamel;
  using namespace RWorker;

  LOG(INFO) << "haRness: main program starting up";

  ConcurrentQueue<std::unique_ptr<RTask>> taskQueue;
  ConcurrentQueue<std::unique_ptr<RResponse>> responseQueue;
  EvalOperationStore operationStore;

  // create worker thread before creating gRPC server
  std::thread rWorkerThread(RWorker::r_worker_thread,
                            std::ref(taskQueue),
                            std::ref(responseQueue));

  REvalServiceImpl rEvalService(std::ref(operationStore),
                                std::ref(taskQueue),
                                std::ref(responseQueue));

  std::string server_address("0.0.0.0:50051");

  grpc::ServerBuilder serverBuilder;
  serverBuilder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  serverBuilder.RegisterService(&rEvalService);
  std::unique_ptr<grpc::Server> server(serverBuilder.BuildAndStart());

  LOG(INFO) << "gRPC Server Listening on " <<server_address;

  server->Wait();

  // will block
  rWorkerThread.join();

  return 0;
}
