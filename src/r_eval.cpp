#include "cpp11/as.hpp"
#include "r_result.h"

#include <R/Rinternals.h>
#include <cpp11.hpp>

#include <iostream>
#include <memory>
#include <chrono>


namespace RWorker {

// need to make an R string, call evaluate on it with the proper parameters
// and then iterate over the evaluate object (either in R or C++) to get
// out the proper strings and graphics objects.
//
// The graphics objects need to be rendered into SVGs first, while iterating
//
// Life will be easier if the iteration can be full 'done' in C++, although
// not the end of the world if not.

// an REvaluator instance is created in the code eval loop
// and stores the state of execution output and constructing the
// RResponse at the end of execution
class REvaluator {
private:
  // vectors that will get passed into
  std::vector<std::string> r_text_output;
  std::vector<std::string> r_plot_output;
  // this will get set to signal the code had an error at some point in
  // execution to allow the agent to retry that specific code instead of
  // increasing context further
  bool eval_error = false;

public:
  REvaluator() = default;

  void process_r_code(const std::string &r_code_snippet) {
    
    // Get R functions using cpp11::package
    cpp11::function evaluate_evaluate = cpp11::package("evaluate")["evaluate"];
    cpp11::function evaluate_trim_plots =
        cpp11::package("evaluate")["trim_intermediate_plots"];
    cpp11::function svglite_svgstring_device_setup =
        cpp11::package("svglite")["svgstring"];
    cpp11::function base_class_fn = cpp11::package("base")["class"];
    cpp11::function base_condition_message =
        cpp11::package("base")["conditionMessage"];
    cpp11::function base_new_env = cpp11::package("base")["new.env"];
    cpp11::function base_exists = cpp11::package("base")["exists"];
    cpp11::function base_get = cpp11::package("base")["get"];
    cpp11::function base_assign = cpp11::package("base")["assign"];

    // more functions for the graphics part
    cpp11::function grdevices_replay_plot =
        cpp11::package("grDevices")["replayPlot"];
    cpp11::function grdevices_dev_off = cpp11::package("grDevices")["dev.off"];
    // get/create the client environment.
    // assumes this cpp11 code is running in the global environment which should
    // be a given

    cpp11::sexp client_r_env_sexp;
    cpp11::r_string client_env_name("client_env");

    if (cpp11::as_cpp<bool>(base_exists(
            client_env_name, cpp11::named_arg("where") = R_GlobalEnv,
            cpp11::named_arg("inherits") = cpp11::r_bool(false)))) {
      client_r_env_sexp =
          base_get(client_env_name, cpp11::named_arg("envir") = R_GlobalEnv,
                   cpp11::named_arg("inherits") = cpp11::r_bool(false));
    } else {
      client_r_env_sexp =
          base_new_env(cpp11::named_arg("parent") = R_GlobalEnv);
      base_assign(client_env_name, client_r_env_sexp,
                  cpp11::named_arg("envir") = R_GlobalEnv);
    }

    try {
      // Evaluate the R code snippet in a new environment
      cpp11::sexp eval_results_sexp = evaluate_evaluate(
          cpp11::r_string(r_code_snippet.c_str()),       // input
          cpp11::named_arg("envir") = client_r_env_sexp, // envir
          cpp11::named_arg("new_device") =
              cpp11::r_bool(true) // new_device (usually default)
      );

      cpp11::sexp trimmed_results_sexp = evaluate_trim_plots(eval_results_sexp);
      cpp11::list r_results(trimmed_results_sexp);

      for (cpp11::sexp r_item_sexp : r_results) {
        if (r_item_sexp == R_NilValue)
          continue;

        cpp11::sexp class_attr_sexp = base_class_fn(r_item_sexp);
        if (class_attr_sexp == R_NilValue || TYPEOF(class_attr_sexp) != STRSXP)
          continue;

        cpp11::strings item_classes(class_attr_sexp);
        if (item_classes.size() == 0)
          continue;

        
        std::string primary_class = static_cast<std::string>(item_classes[0]);

        if (primary_class == "source") {
          cpp11::list item_as_list(r_item_sexp);

          // Default evaluate source handler creates list(src = value)
          // where value is the source code character vector.

          cpp11::sexp src_val_sexp = item_as_list["src"];

          if (src_val_sexp == R_NilValue) {
            continue;
          }
          if (TYPEOF(src_val_sexp) == STRSXP) {
            cpp11::strings src_lines(src_val_sexp);
            for (const cpp11::r_string &line : src_lines) {
              r_text_output.push_back("> " + static_cast<std::string>(line));
            }
          }
        } else if (primary_class ==
                   "character") { // Plain text output from print(), cat()
          cpp11::strings lines(r_item_sexp);
          for (const cpp11::r_string &line : lines) {
            r_text_output.push_back(static_cast<std::string>(line));
          }
        } else if (primary_class == "recordedplot") {
          try {
            // need to call svgstring() first to setup the device
            // also able to set the svgstring parameters here, like width/height
            cpp11::sexp svgstring_capture = svglite_svgstring_device_setup();
            cpp11::function svg_capture_func(svgstring_capture);

            // replay the plot
            grdevices_replay_plot(r_item_sexp);

            // call the capture function
            cpp11::sexp svg_captured_content = svg_capture_func();

            // close device
            grdevices_dev_off();

            // a char vector returned
            if (TYPEOF(svg_captured_content) == STRSXP &&
                cpp11::strings(svg_captured_content).size() > 0) {
              cpp11::strings svg_r_string(svg_captured_content);
              r_plot_output.push_back(
                  static_cast<std::string>(svg_r_string[0]));
            } else {
              r_text_output.push_back("Warning: svglite::svgstring capturer "
                                      "function didn't return a STRSXP");
            }
          } catch (const cpp11::unwind_exception &e_svg) {
            eval_error = true;
            r_text_output.push_back(
                "Error: Failed to convert plot to SVG with svglite.");
            // Potentially log more details from e_svg if possible, or let outer
            // handler do it. For now, the R error message from svglite might be
            // caught by R_ContinueUnwind by the caller
          } catch (const std::exception &e_svg_cpp) {
            eval_error = true;
            r_text_output.push_back(
                std::string("Error: C++ exception during SVG conversion: ") +
                e_svg_cpp.what());
          }
        } else if (primary_class == "error" || primary_class == "simpleError" || 
                   item_classes.contains(cpp11::r_string("error"))) {
          eval_error = true;
          cpp11::sexp error_msg_sexp = base_condition_message(r_item_sexp);
          if (TYPEOF(error_msg_sexp) == STRSXP &&
              cpp11::strings(error_msg_sexp).size() > 0) {
            cpp11::strings error_message_r(error_msg_sexp);
            r_text_output.push_back(
                "Error: " + static_cast<std::string>(error_message_r[0]));
          } else {
            r_text_output.push_back("Error: An R error occurred, but its "
                                    "message could not be retrieved.");
          }
        } else if (primary_class == "warning" || primary_class == "simpleWarning" ||
                   item_classes.contains(cpp11::r_string("warning"))) {
          cpp11::sexp warning_msg_sexp = base_condition_message(r_item_sexp);
          if (TYPEOF(warning_msg_sexp) == STRSXP &&
              cpp11::strings(warning_msg_sexp).size() > 0) {
            cpp11::strings warning_message_r(warning_msg_sexp);
            r_text_output.push_back(
                "Warning: " + static_cast<std::string>(warning_message_r[0]));
          } else {
            r_text_output.push_back("Warning: An R warning occurred, but its "
                                    "message could not be retrieved.");
          }
        } else if (primary_class == "message" || primary_class == "simpleMessage" || 
                   item_classes.contains(cpp11::r_string("message"))) {
          cpp11::sexp message_msg_sexp = base_condition_message(r_item_sexp);
          if (TYPEOF(message_msg_sexp) == STRSXP &&
              cpp11::strings(message_msg_sexp).size() > 0) {
            cpp11::strings message_r(message_msg_sexp);
            r_text_output.push_back(static_cast<std::string>(message_r[0]));
          } else {
            r_text_output.push_back("Message: An R message occurred, but its "
                                    "content could not be retrieved.");
          }
        }
        // Other types like "evaluate_restarting" can be ignored for output
        // capture.
      }
    
    } catch (const cpp11::unwind_exception &e) {
      eval_error = true;
      // This indicates an error in the R API calls themselves (evaluate,
      // trim_plots, etc.) The actual R error message will typically be printed
      // by R's top-level error handler when R_ContinueUnwind is called by the
      // wrapper of this C++ code. We can try to get the last R error message to
      // include it in our text output.
      r_text_output.push_back(
          "Error: R API call failed (cpp11::unwind_exception).");
      try {
        cpp11::function get_r_error_message =
            cpp11::package("base")["geterrmessage"];
        cpp11::strings r_error_msg_sxp(get_r_error_message());
        if (r_error_msg_sxp.size() > 0) {
          r_text_output.push_back(
              cpp11::as_cpp<std::string>(r_error_msg_sxp[0]));
        }
      } catch (...) {
        // Failed to get error message, do nothing extra.
      }
      // It's crucial that the caller of `process_r_code` handles or
      // re-throws/continues the unwind. This function will just populate
      // outputs and set eval_error.
    } catch (const std::exception &e) {
      eval_error = true;
      r_text_output.push_back(
          std::string("Error: C++ exception during R processing: ") + e.what());
    }
  }

  // we want each section to be its own line so we strip the extra newline
  // after each line in the r_text_output vector
  void strip_trailing_newline() {
    for(std::string& line : r_text_output) {
      if (!line.empty() && line[line.length()-1] == '\n') {
        line.erase(line.length() - 1);
      }
    }
  }

  RResponse build_response(const std::string &task_uuid) const {
    RClientOutputPayload payload;
    payload.console_output = r_text_output;
    payload.graphic_output = r_plot_output;

    ResponseStatus status = eval_error ? ResponseStatus::FAILURE_R_SCRIPT_ERROR
                                       : ResponseStatus::SUCCESS;

    // The error message for R script errors is part of the console_output.
    // The RResponse's optional error_message is more for C++ or task-level
    // errors outside R.
    return RResponse(task_uuid, status, payload);
  }

  bool has_error() const { return eval_error; }
};

std::unique_ptr<RResponse> eval_client_R(std::string code,
                                         std::string task_uuid) {
  // debug print
  #ifndef NDEBUG
  std::cout << "eval_client_R: " << __FILE__ << '\n'
            << '\t' << "code: " << code << '\n'
            << std::flush;
  #endif

  REvaluator evaluator;

  // call on the code
  evaluator.process_r_code(code);

  // strip trailing newline
  evaluator.strip_trailing_newline();

  // get the RResponse back and set the task_uuid &&&&& make a unique ptr!! :)
  return std::make_unique<RResponse>(evaluator.build_response(task_uuid));
}

} // namespace RWorker
