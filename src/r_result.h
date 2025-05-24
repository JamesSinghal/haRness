#pragma once

#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace RWorker {

enum class ResponseStatus {
  // normal success
  SUCCESS,
  // R script execution error, should be tied to helpful
  // error description from the R code itself in the output
  // text
  FAILURE_R_SCRIPT_ERROR,
  // Probably unused for now, failure of View() or grabbing var value
  FAILURE_R_VIEW_ERROR,
  // error in CPP command
  FAILURE_CPP_COMMAND,
  // catch all other failure
  FAILURE_TASK_EXECUTION,
  // not used for now
  FAILURE_TIMEOUT,
  // parsing error? probably unused, going to get caught by R_SCRIPT_ERROR
  FAILURE_INVALID_TASK
};

// result data containers
struct RClientOutputPayload {
  // all of these should be in their output order based on the R code
  // captured text and source output from evaluate obj parsing
  std::vector<std::string> console_output;
  // captured SVG (for now) output of plots
  // TODO: think about compression for these, brotli prob or zstd (faster)
  //       or let the network thread compress. premature opt
  std::vector<std::string> graphic_output;
};

// payload for cpp management tasks
struct ManagementTaskResultPayload {
  // super vague for now, probably just for debugging for now
  // the actual success/failure is encoded in the response status
  //
  // can be blank
  std::string result_message;
};

// variant for these two and the possibility of neither (for the error types)
using ResultData = std::variant<std::monostate, RClientOutputPayload,
                                ManagementTaskResultPayload>;

class RResponse {
public:
  // public constructor that creates a response with:
  // - the given UUID, matching the task
  // - status from the `ReponseStatus` enum
  // - `ResultData` variant payload (optional)
  // - error message (optional)
  RResponse(std::string task_uuid, ResponseStatus status,
            ResultData payload = std::monostate{},
            std::optional<std::string> error_message = std::nullopt);

  // getters and helper methods
  const std::string &get_task_uuid() const { return task_uuid_; }
  ResponseStatus get_status() const { return status_; }
  const std::optional<std::string> &get_error_message() const {
    return error_message_;
  }
  const ResultData &get_result_payload() const { return result_payload_; }

  bool is_success() const { return status_ == ResponseStatus::SUCCESS; }

  // could add future convenience getters based on the variant type of
  // ResultData

private:
  std::string task_uuid_;
  ResponseStatus status_;
  std::optional<std::string> error_message_;
  ResultData result_payload_;
};

// debug print overload
std::ostream &operator<<(std::ostream &os, const RResponse &response);

} // namespace RWorker
