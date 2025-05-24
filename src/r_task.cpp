#include <r_task.h>

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

namespace RWorker {

std::string generate_uuid_for_rtask() {
  static std::mt19937_64 generator(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  std::uniform_int_distribution<uint64_t> distribution;

  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < 2; ++i) { // generate a 32-character hex string (128-bit)
    ss << std::setw(16) << distribution(generator);
  }

  return ss.str();
}

RTask::RTask(TaskType type, TaskData data)
    : uuid_(generate_uuid_for_rtask()), type_(type), data_(std::move(data)) {}

// factory constructors, public
std::unique_ptr<RTask> RTask::create_client_r_code_task(std::string r_code) {
  // new RTask(...) calls the private constructor, which is allowed for static
  // members.
  return std::unique_ptr<RTask>(new RTask(TaskType::EXECUTE_R_CODE_CLIENT,
                                          RCodePayload{std::move(r_code)}));
}

std::unique_ptr<RTask>
RTask::create_management_r_code_task(std::string r_code) {
  return std::unique_ptr<RTask>(new RTask(TaskType::EXECUTE_R_CODE_MANAGEMENT,
                                          RCodePayload{std::move(r_code)}));
}

std::unique_ptr<RTask>
RTask::create_cpp_management_task(std::string command_id,
                                  std::vector<std::string> arguments) {
  return std::unique_ptr<RTask>(new RTask(
      TaskType::CPP_MANAGEMENT_TASK,
      CppManagementPayload{std::move(command_id), std::move(arguments)}));
}

// overload for debug printing / logging
std::ostream &operator<<(std::ostream &os, const RTask &task) {
  os << "RTask {" << std::endl;
  os << "  UUID: " << task.uuid_ << std::endl;

  os << "  Type: ";
  switch (task.type_) {
  case TaskType::EXECUTE_R_CODE_CLIENT:
    os << "EXECUTE_R_CODE_CLIENT";
    break;
  case TaskType::EXECUTE_R_CODE_MANAGEMENT:
    os << "EXECUTE_R_CODE_MANAGEMENT";
    break;
  case TaskType::CPP_MANAGEMENT_TASK:
    os << "CPP_MANAGEMENT_TASK";
    break;
  default:
    // Print the underlying integer value if the enum value is not recognized
    os << "UNKNOWN_TASK_TYPE (value: " << static_cast<int>(task.type_) << ")";
    break;
  }
  os << std::endl;

  os << "  Data: {" << std::endl;
  std::visit(
      [&os](const auto &payload) {
        // std::decay_t and std::is_same_v are used to check the type of the
        // active variant member
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, RCodePayload>) {
          os << "    RCodePayload: {" << std::endl;
          // For potentially long or multi-line code, this simple print is a
          // starting point. More sophisticated formatting (e.g., indenting
          // multi-line code) could be added if needed.
          os << "      Code: \"" << payload.code << "\"" << std::endl;
          os << "    }" << std::endl;
        } else if constexpr (std::is_same_v<T, CppManagementPayload>) {
          os << "    CppManagementPayload: {" << std::endl;
          os << "      Command Identifier: \"" << payload.command_identifier
             << "\"" << std::endl;
          os << "      Arguments: [";
          bool first_arg = true;
          for (const auto &arg : payload.arguments) {
            if (!first_arg) {
              os << ", ";
            }
            // Arguments are printed as quoted strings.
            os << "\"" << arg << "\"";
            first_arg = false;
          }
          os << "]" << std::endl;
          os << "    }" << std::endl;
        }
        // No 'else' branch is needed here because std::visit guarantees that
        // 'payload' will be one of the types specified in the TaskData variant.
      },
      task.data_);
  os << "  }" << std::endl; // End of Data section

  os << "}";       // End of RTask object
  os << std::endl; // Add a final newline to ensure multiple RTask prints are
                   // well-separated.

  return os;
}

} // namespace RWorker
