#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>

#include "envs.h"
#include "r_eval.h"
#include "r_init.h"
#include "r_result.h"
#include "r_task.h"
#include "r_worker.h"

// R includes
#include <R.h>
#include <Rembedded.h>
// define R_INTERFACE_PTRS to get access to the callback ptrs
#define R_INTERFACE_PTRS 1
#define CSTACK_DEFNS
#include <Rinternals.h>

// Rinternals.h MUST be defined first for the SEXP definition
// this comment is to prevent clang-format from rearranging

#include <R_ext/Parse.h>
#include <Rinterface.h>

// R_HOME_DEFINE
#ifndef R_HOME_CMAKE
#define R_HOME_CMAKE "/fake/path/should/fail"
#endif

// concurrent queue include
#include <concurrentqueue.h>

// logging/check
#include <absl/log/check.h>

namespace RWorker {

bool is_R_init = false;

using namespace moodycamel;

void r_worker_thread(
    std::stop_token stop_token,
    ConcurrentQueue<std::unique_ptr<RTask>> &taskQueue,
    ConcurrentQueue<std::unique_ptr<RResponse>> &responseQueue) {
  // set R_HOME and R_LIBS
  try {
    set_r_home(R_HOME_CMAKE);
  } catch (const std::exception &error) {
    std::cerr << "Could not set R_HOME because of error:" << std::endl;
    std::cerr << "\tException::what() -- " << error.what() << std::endl;
    std::terminate();
  }

  R_SignalHandlers = 0;

  // options for R, --gui=none does not seem to be respected
  char *argv[] = {(char *)"haRness", (char *)"--gui=none", (char *)"--silent"};

  // Code below is the same as Rf_initEmbeddedR, but due to the
  // stack checking problem, as mentioned in "Writing R Extensions"
  // we need to disable stack checking *before* we call setup_Rmainloop()
  // Rf_initEmbeddedR(sizeof(argv) / sizeof(argv[0]), argv);
  Rf_initialize_R(sizeof(argv) / sizeof(argv[0]), argv);
  R_CStackLimit = (uintptr_t)-1;
  R_Interactive = TRUE;
  setup_Rmainloop();
  // set global
  is_R_init = true;

  // run R setup snippets
  // TODO: write better error handling for this
  if (!exec_R_setup()) {
    throw std::logic_error("R setup snippets did not run correctly");
  }

  while (!stop_token.stop_requested()) {
    // TODO: actually write task handling
    std::unique_ptr<RTask> task;

    if (taskQueue.try_dequeue(task)) {
      // std::cout << "R thread: time: "
      //          << std::chrono::high_resolution_clock().now() << std::endl;

      if (task.get()) {
        // std::cout << "Valid ptr" << std::endl;
        // std::cout << *task << std::endl;

        switch (task->get_type()) {
        case TaskType::EXECUTE_R_CODE_CLIENT: {
          std::string task_uuid = task->get_uuid();
          TaskData task_data = task->get_data();

          std::string r_code = std::get<RCodePayload>(task_data).code;

          std::unique_ptr<RResponse> client_eval_response =
              eval_client_R(r_code, task_uuid);

          responseQueue.enqueue(std::move(client_eval_response));
          break;
        }
        case TaskType::EXECUTE_R_CODE_MANAGEMENT:
          break;
        case TaskType::CPP_MANAGEMENT_TASK:
          break;
        default:
          // should be unreachable
          CHECK(false) << "RTask of unknown type";
          break;
        }

      } else {
        // use abseil check for invariant
        CHECK(task.get()) << "RTask dequeue unique_ptr null";
      }

      // if there was a task, we continue to grab the next immediately
      // if not we wait for 1 second
      continue;
    }

    // artificial slowdown for debug
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // cleanup
  Rf_endEmbeddedR(0);
}

} // namespace RWorker
