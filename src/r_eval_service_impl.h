#pragma once

#include <grpcpp/grpcpp.h>
#include "r_result.h"
#include "reval_service.grpc.pb.h"
#include "operation_store.h"
#include "r_task.h"
#include "reval_service.pb.h"

#include <stop_token>
#include <thread>
#include <concurrentqueue.h>

class REvalServiceImpl final : public REvalService::CallbackService {
public:
  // constructor with references to the task queue and op store
  // will need to add anything that will need to be accessed in the
  // grpc handlers
  explicit REvalServiceImpl(
    EvalOperationStore& operation_store,
    moodycamel::ConcurrentQueue<std::unique_ptr<RWorker::RTask>>& task_queue,
    moodycamel::ConcurrentQueue<std::unique_ptr<RWorker::RResponse>>& response_queue
  ) : operation_store_(operation_store),
    task_queue_(task_queue), response_queue_(response_queue),
    response_thread_(&REvalServiceImpl::ProcessRResponseQueue, this) {}

  // actual rpc handler signatures
  grpc::ServerUnaryReactor* EvalRScript(
    grpc::CallbackServerContext* context,
    const EvalRScriptRequest* request,
    EvalOperation* response) override;

  grpc::ServerUnaryReactor* GetEvalOperation(
    grpc::CallbackServerContext* context,
    const GetEvalOperationRequest* request,
    EvalOperation* response) override;
  
  grpc::ServerUnaryReactor* CancelEvalOperation(
    grpc::CallbackServerContext* context,
    const CancelEvalOperationRequest* request,
    google::protobuf::Empty* response) override;

private:
  EvalOperationStore& operation_store_;
  moodycamel::ConcurrentQueue<std::unique_ptr<RWorker::RTask>>& task_queue_;
  moodycamel::ConcurrentQueue<std::unique_ptr<RWorker::RResponse>>& response_queue_;
  // bg task
  std::jthread response_thread_;

  // private function to be called as a thread that will get responses off of the
  // response queue and modify the operation store based on the contents
  //
  // TODO: consider how to handle queue type errors (i.e. two responses for one task)
  //        -- realistically shouldn't happen
  void ProcessRResponseQueue(std::stop_token stop_token);
};
