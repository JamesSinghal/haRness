#include "r_result.h"
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <variant>

namespace RWorker {

RResponse::RResponse(std::string task_uuid, ResponseStatus status,
                     // pass by value to be moved
                     ResultData payload,
                     // optional by value to be moved
                     std::optional<std::string> error_message)
    : task_uuid_(std::move(task_uuid)), status_(status),
      error_message_(std::move(error_message)),
      result_payload_(std::move(payload)) {}

// probably want helpful factory methods for constructing ResponseData and maybe
// the others, or maybe even a full response

// in terms of flows, for R code we want to be able to create the shell
// with the uuid, then add text and graphic responses in as we go.
//
// it could go into an error state after a couple lines are successful,
// but after that it will be stuck in error

// Overload for debug printing / logging
std::ostream &operator<<(std::ostream &os, const RResponse &response) {
  os << "RResponse {" << std::endl;
  os << "  Task UUID: " << response.get_task_uuid() << std::endl; //

  os << "  Status: ";
  switch (response.get_status()) { //
  case ResponseStatus::SUCCESS:
    os << "SUCCESS"; //
    break;
  case ResponseStatus::FAILURE_R_SCRIPT_ERROR:
    os << "FAILURE_R_SCRIPT_ERROR"; //
    break;
  case ResponseStatus::FAILURE_R_VIEW_ERROR:
    os << "FAILURE_R_VIEW_ERROR"; //
    break;
  case ResponseStatus::FAILURE_CPP_COMMAND:
    os << "FAILURE_CPP_COMMAND"; //
    break;
  case ResponseStatus::FAILURE_TASK_EXECUTION:
    os << "FAILURE_TASK_EXECUTION"; //
    break;
  case ResponseStatus::FAILURE_TIMEOUT:
    os << "FAILURE_TIMEOUT"; //
    break;
  case ResponseStatus::FAILURE_INVALID_TASK:
    os << "FAILURE_INVALID_TASK"; //
    break;
  default:
    os << "UNKNOWN_RESPONSE_STATUS (value: "
       << static_cast<int>(response.get_status()) << ")"; //
    break;
  }
  os << std::endl;

  if (response.get_error_message().has_value()) { //
    os << "  Error Message: \"" << response.get_error_message().value()
       << "\"" //
       << std::endl;
  }

  os << "  Result Payload: {" << std::endl;
  std::visit( //
      [&os](const auto &payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, std::monostate>) { //
          os << "    Monostate" << std::endl;
        } else if constexpr (std::is_same_v<T, RClientOutputPayload>) { //
          os << "    RClientOutputPayload: {" << std::endl;
          os << "      Console Output: [" << std::endl;
          for (const auto &line : payload.console_output) { //
            // Simple print, consider escaping newlines or truncating long lines
            // if necessary.
            os << "        \"" << line << "\"" << std::endl;
          }
          os << "      ]" << std::endl;
          os << "      Graphic Output: [" << std::endl;
          for (const auto &svg_data : payload.graphic_output) { //
            // Print a placeholder or summary for graphic data to avoid overly
            // long output.
            os << "        SVG Data (size: " << svg_data.length() << " bytes)"
               << std::endl;
            // Alternatively, print a snippet:
            // os << "        \"" << svg_data.substr(0, 50) <<
            // (svg_data.length() > 50 ? "..." : "") << "\"" << std::endl;
          }
          os << "      ]" << std::endl;
          os << "    }" << std::endl;
        } else if constexpr (std::is_same_v<T,
                                            ManagementTaskResultPayload>) { //
          os << "    ManagementTaskResultPayload: {" << std::endl;
          os << "      Result Message: \"" << payload.result_message << "\"" //
             << std::endl;
          os << "    }" << std::endl;
        }
      },
      response.get_result_payload()); //
  os << "  }" << std::endl;           // End of Result Payload section

  os << "}";       // End of RResponse object
  os << std::endl; // Add a final newline

  return os;
}

} // namespace RWorker
