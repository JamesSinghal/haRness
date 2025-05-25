#include "r_eval_service_impl.h"

// the constructor just sets in the initializer list

// the EvalRScript rpc should get the code, create the task
// and add the task to the member variable task queue
// and then construct the evaloperation in the EvalOperationStore
// then send it back to the client

// the getevaloperation should just lookup the eval and send it back

// the cancel should be a no-op for now

// therefore we also need a thread whos only job is to handle grabbing
// responses off the response queue and modifying the EvalOperationStore
// based on the response data.
//
// need to think about the future having more response types and if we
// want just one service handling every possible type of response type

grpc::ServerUnaryReactor *EvalRScript(grpc::CallbackServerContext *context,
                                      const EvalRScriptRequest *request,
                                      EvalOperation *response) {}

grpc::ServerUnaryReactor *
GetEvalOperation(grpc::CallbackServerContext *context,
                 const GetEvalOperationRequest *request,
                 EvalOperation *response) {}

grpc::ServerUnaryReactor *
CancelEvalOperation(grpc::CallbackServerContext *context,
                    const CancelEvalOperationRequest *request,
                    google::protobuf::Empty *response) {}
