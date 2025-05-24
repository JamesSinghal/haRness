#include "r_init.h"
#include "r_worker.h"
#include <iostream>
#include <stdexcept>

#include <cpp11.hpp>

namespace RWorker {

static bool exec_R_snippet(std::string const &code) {
  // skip empty
  if (code.empty())
    return true;

  std::cout << "RWorker Setup, snippet: " << code << std::endl;

  try {
    cpp11::package base_pkg("base");
    cpp11::function r_parse = base_pkg["parse"];
    cpp11::function r_eval = base_pkg["eval"];

    // parse the code, cpp11::sexp automatically protects the value
    cpp11::sexp r_snippet_parsed =
        r_parse(cpp11::named_arg("text") = code.c_str());

    // eval the parsed R snippet
    cpp11::sexp r_result = r_eval(r_snippet_parsed);

    return true;
  } catch (const std::exception &e) {
    std::cerr << "RWorker: R setup snippet execution error " << e.what()
              << std::endl;
    return false;
  }

  return true;
}

class RSetup {
public:
  static RSetup &getInstance() {
    static RSetup instance;
    return instance;
  }

  void register_r_snippet(std::string code) { r_snippets_.push_back(code); }

  bool exec_R_snippets() {
    for (const std::string &code : r_snippets_) {
      if (!exec_R_snippet(code)) {
        return false;
      }
    }

    return true;
  }

  // R Init Snippets need to be added here
  RSetup() {
    r_snippets_.push_back("library(data.table)");
    r_snippets_.push_back("library(ggplot2)");
    r_snippets_.push_back("library(svglite)");
    r_snippets_.push_back("library(evaluate)");
    r_snippets_.push_back("library(dplyr)");

    // set default ggplot2 theme
    r_snippets_.push_back("theme_set(theme_bw())");

    r_snippets_.push_back("options(device = \"svglite\")");

    r_snippets_.push_back("print(\"setup done!\")");
  }

private:
  std::vector<std::string> r_snippets_;
};

bool exec_R_setup() {
  if (!is_R_init) {
    throw std::logic_error("R not initialized before calling exec_R_setup()");
  }

  std::cout << "Starting R setup: exec_R_setup()" << std::endl;

  RSetup &r_setup = RSetup::getInstance();

  // running actual snippets
  if (!r_setup.exec_R_snippets()) {
    throw std::runtime_error("RSetup snippets failed to run completely");
  }

  return true;
}

} // namespace RWorker
