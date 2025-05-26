#include "r_eval_service_impl.h"
#include "r_result.h"
#include "reval_service.pb.h"
#include <grpcpp/support/status.h>
#include <absl/log/log.h>
#include <thread>

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
//
// MEMBER VARIABLES:
// EvalOperationStore& operation_store_; -- Contains the thread-safe map UUID ->
// EvalOperation protobuf moodycamel::ConcurrentQueue<RWorker::RTask>&
// task_queue_; -- Send tasks to R thread

grpc::ServerUnaryReactor *REvalServiceImpl::EvalRScript(grpc::CallbackServerContext *context,
                                      const EvalRScriptRequest *request,
                                      EvalOperation *response)
{
    // get the requested R code string from the request message
    std::string r_code = request->r_code();

    // construct an RTask with the factory method
    std::unique_ptr<RWorker::RTask> r_task = RWorker::RTask::create_client_r_code_task(r_code);

    // grab the name_uuid from the created RTask
    std::string eval_uuid = r_task->get_uuid();

    // We have the necessary data, now we need to do two things:
    // 1. Add the RTask to the TaskQueue
    // 2. Construct the Operation in the EvalOperationStore
    // 3. Construct the Response to the gRPC RPC
    //
    // Do this in order to see that the R code might be done faster?
    // Otherwise there is no neccesary/strict ordering

    // enqueue the R Code Eval Task
    task_queue_.enqueue(std::move(r_task));

    // next, construct the operation in the operation store
    // we need the name/uuid to index. creation time is automatically
    // set by the class and otherwise we just need to set the done to false
    EvalOperation temp_operation = operation_store_.createEvalOperation(eval_uuid);

    // the operation is already in the operation store and the start time
    // is already set, so we need to set the response and then return.
    response->CopyFrom(temp_operation);

    // use simple example from docs with default Reactor
    // we might move to a custom reactor if we need to customize behavior
    // of OnDone, OnCancel, etc... or if streaming
    auto* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}

grpc::ServerUnaryReactor *
REvalServiceImpl::GetEvalOperation(grpc::CallbackServerContext *context,
                 const GetEvalOperationRequest *request,
                 EvalOperation *response)
{
    // this needs to just get the message out of the operation store
    // and copy it into the response object. for now, the assumption
    // that must be followed is that someone (thread) else *must*
    // maintain the status of the operation in the operation store
    // to be correct.
    std::string requested_uuid = request->name();

    // of course we return not-found if we do not find it
    auto eval_operation = operation_store_.getEvalOperation(requested_uuid);

    auto* reactor = context->DefaultReactor();
    if (eval_operation.has_value()) {
        // eval operation exists in store
        response->CopyFrom(eval_operation.value());
        reactor->Finish(grpc::Status::OK);
        return reactor;
    } else {
        // eval operation *does NOT* exist in store
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Operation doesn't exist in store"));
        return reactor;
    }
}

grpc::ServerUnaryReactor *
REvalServiceImpl::CancelEvalOperation(grpc::CallbackServerContext *context,
                    const CancelEvalOperationRequest *request,
                    google::protobuf::Empty *response)
{
    // unimplemented
    auto* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "CancelEvalOperation is not implemented yet. It does nothing."));
    return reactor;
}

void REvalServiceImpl::ProcessRResponseQueue(std::stop_token stop_token) {
    // this will be called and have access to the queue and operation store
    //
    // we need to work in a busy loop and dequeue RResponses off of the
    // response queue, modify the operation store, and check if the stop_token
    // has been signaled.
    //
    // to check the stop_token, call stop_token.stop_requested()
    LOG(INFO) << "RResponse queue processing thread starting up!";

    // while loop that should be gated on the stop_token
    while(!stop_token.stop_requested()) {
        // if in here we are alive
        // we need to:
        // 1. is a response ready
        // 2. sleep

        std::unique_ptr<RWorker::RResponse> r_response;
        if (response_queue_.try_dequeue(r_response)) {
            // if we're here we have a dequeued r_response

            std::string eval_uuid = r_response->get_task_uuid();
            RWorker::ResponseStatus eval_status = r_response->get_status();
            RWorker::ResultData eval_data = r_response->get_result_payload();

            LOG(INFO) << "RResponse Status: " << eval_status << "gotten off of queue.";

            // we have the uuid and can call updateEvalOperation
            operation_store_.updateEvalOperation(eval_uuid,
                [&](EvalOperation& op_protobuf) {
                    switch (eval_status) {
                        // right now we are only sending R code
                        // as client code, so it will either be success
                        // failure w/ error, or we just mark it as done
                        // and be done with it. later we will add handling
                        case RWorker::ResponseStatus::SUCCESS:
                        {
                            std::vector<std::string> output_text = std::get<RWorker::RClientOutputPayload>(eval_data).console_output;

                            std::vector<std::string> svg_text = std::get<RWorker::RClientOutputPayload>(eval_data).graphic_output;

                            // we have the output, now add it to the proto
                            auto eval_result_pbuf = op_protobuf.mutable_eval_result();

                            // set status, need to set lines too
                            eval_result_pbuf->set_status(EVAL_SUCCESS);

                            eval_result_pbuf->clear_interpreter_lines();
                            for(std::string& line : output_text) {
                                eval_result_pbuf->add_interpreter_lines(line);
                            }

                            eval_result_pbuf->clear_svg_plots();
                            for(std::string& svg : svg_text) {
                                eval_result_pbuf->add_svg_plots(svg);
                            }

                            op_protobuf.set_done(true);
                            break;
                        }
                        case RWorker::ResponseStatus::FAILURE_R_SCRIPT_ERROR:
                        {
                            std::vector<std::string> output_text = std::get<RWorker::RClientOutputPayload>(eval_data).console_output;

                            std::vector<std::string> svg_text = std::get<RWorker::RClientOutputPayload>(eval_data).graphic_output;

                            // we have the output, now add it to the proto
                            auto eval_result_pbuf = op_protobuf.mutable_eval_result();

                            // set status, need to set lines too
                            eval_result_pbuf->set_status(EVAL_R_CODE_ERROR);

                            for(std::string& line : output_text) {
                                eval_result_pbuf->add_interpreter_lines(line);
                            }

                            for(std::string& svg : svg_text) {
                                eval_result_pbuf->add_svg_plots(svg);
                            }

                            op_protobuf.set_done(true);
                            break;
                        }
                        default:
                        {
                            op_protobuf.set_done(true);
                            //TODO: handle this
                            LOG(WARNING) << "Response Status Not Implemented";
                            break;
                        }
                    }
                });

            // if there was a task we immediately get another
            // if not we wait 20 milli
            continue;
        }

        // sleep :)
        // this should possibly be configurable or a better solution be found?
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    LOG(INFO) << "RResponse queue processing thread shutting down!";
}
