#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

// RTask encapsulates something that the R thread should "do"
//
// Each task will have some sort of uuid which will be tagged to the task
// and the corresponding response will carry that same uuid to allow the main
// thread to connect them. This might end up being more important later on
// but since only one task should really be able to be executed at a time
// (one R worker thread), the responses should be in order.
//
// it is somewhat generic on purpose, and might use std::variant in
// the future for encapsulting different types of "work" as they
// arise, but the first and most obvious is:
// 1. Client code to run in the R environment (capturing output + graphics)
//
// Second, we might need to execute code for management, View(), generally
// stuff not driven by the LLM Agent loop but more deterministic stuff.
// View() is a good example, as is a front-end interface that says "Load X
// dataset". Unambiguously saying, load this dataset (of course with the
// required source, etc..) Therefore, no LLM input or refinement needed, and
// really the only response needed is success/failure. RTask is only concerned
// with the task though, not the response
// 2. R environment management code as described above.
//
// Third, we probably need management tasks for the R thread to execute
// *outside* of R, thus not R code but an instruction that dedicated C++ code
// will handle, still in the R thread.
//
// Summary:
// 1. Client R code (from LLM or client)
// 2. Deterministic mgmt R code (generated code from Front-end or otherwise)
// 3. Management task for C++ to execute, probably in a switch case (*not* R)
// 4. ?
//
// The non-R tasks for the R worker should be for tasks that have to be executed
// synchronously with the R process (i.e. cannot be done at the same time) while
// the R thread is executing code. otherwise the network thread could
// theoretically handle it.
//
// For example, the network thread could handle listing working directory files
// on it's own (async), but could *not* handle getting the value of an R
// variable on it's own.

namespace RWorker {

enum class TaskType {
  EXECUTE_R_CODE_CLIENT,
  EXECUTE_R_CODE_MANAGEMENT,
  CPP_MANAGEMENT_TASK
};

struct RCodePayload {
  std::string code;
  // in the future might consider some further options:
  // e.g. bool expect_graphics_output, std::string plot_theme
};

struct CppManagementPayload {
  std::string command_identifier;
  // in future might be more expicit about the above, make it an enum as well?
  // flexible for now because don't know exactly what might go here
  // Need to brainstorm:
  // - echo - for testing, sends back the first string in arguments or error if
  // empty
  //
  std::vector<std::string> arguments;
  // could use the above for more flexible processing, even with enum
  // just opens the door for failure/parsing mistakes
  // if they end up being pairs, just use a vector of string pairs to avoid
  // having to split them
};

using TaskData = std::variant<RCodePayload, CppManagementPayload>;

class RTask {
public:
  // factory constructors
  static std::unique_ptr<RTask> create_client_r_code_task(std::string r_code);
  static std::unique_ptr<RTask>
  create_management_r_code_task(std::string r_code);
  static std::unique_ptr<RTask>
  create_cpp_management_task(std::string command_id,
                             std::vector<std::string> arguments = {});

  const std::string &get_uuid() const { return uuid_; }
  TaskType get_type() const { return type_; }
  const TaskData &get_data() const { return data_; }

  // overload for debug printing/logging, necessary for access to private vars
  friend std::ostream &operator<<(std::ostream &os, const RTask &task);

private:
  // private ctor to force use of the factory methods
  RTask(TaskType type, TaskData data);

  // member vars
  std::string uuid_;
  TaskType type_;
  TaskData data_;
};

std::ostream &operator<<(std::ostream &os, const RTask &task);

} // namespace RWorker
