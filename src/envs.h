#pragma once

#include <filesystem>

// need to set the env var R_HOME to the correct local path based on PWD
// assumption: R installed to r-install/ relative folder to PWD
// assumption: R_HOME can be set to r-install/lib/R/
// should throw if R isn't there so we need a way to check
void set_r_home(const std::filesystem::path &path_to_rhome);
