#pragma once

#include <grpcpp/grpcpp.h>
#include "reval_service.grpc.pb.h"
#include "operation_store.h"
#include "r_task.h"
#include "reval_service.pb.h"

#include <concurrentqueue.h>

class REvalServiceImpl final : public REvalService::CallbackService {
public:
  // constructor with references to the task queue and op store
  // will need to add anything that will need to be accessed in the
  // grpc handlers
  explicit REvalServiceImpl(
    EvalOperationStore& operation_store,
    moodycamel::ConcurrentQueue<RWorker::RTask>& task_queue
  ) : operation_store_(operation_store),
    task_queue_(task_queue) {}

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
  moodycamel::ConcurrentQueue<RWorker::RTask>& task_queue_;
};
