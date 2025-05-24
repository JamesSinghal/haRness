#include <filesystem>
#include <iostream>

void check_and_set_rhome(const std::filesystem::path &r_home_path) {
  namespace fs = std::filesystem;
  // we want a way to check the desired R_HOME path to sanity check it for being
  // a valid R_HOME
  //
  // want to check: R_HOME path is absolute
  //                R_HOME path is a directory that exists
  //                lib/libR.so exists as shared lib
  //                library/base/ exists as directory
  //
  // throw exception if they don't

  bool r_home_absolute = r_home_path.is_absolute();
  bool r_home_is_dir = fs::is_directory(r_home_path);

  if (!r_home_absolute)
    throw std::runtime_error("R_HOME is not an absolute path");
  if (!r_home_is_dir)
    throw std::runtime_error("R_HOME is not a directory that exists");

  // libR exists
  fs::path libR_path = r_home_path / "lib/libR.so";
  bool libR_exists = fs::is_regular_file(libR_path);

  if (!libR_exists)
    throw std::runtime_error("libR.so not found in R_HOME/lib/");

  // base library installed and found
  fs::path base_lib_path = r_home_path / "library/base/";
  bool base_lib_exists = fs::is_directory(base_lib_path);

  if (!base_lib_exists)
    throw std::runtime_error("base library not found in R_HOME");

  // if we've gotten to this point, we can be relatively confident
  // that `r_home_path` is a valid absolute path to the R installation
  // TODO: figure out a way to ensure the R_HOME installation is v4.5.0
  //       backup is to use the SVN_REVISION

  // set the environmental variable here
  if (setenv("R_HOME", r_home_path.c_str(), 1) != 0) {
    throw std::runtime_error(
        "setenv() failed on set R_HOME, returned non-zero");
  }

  fs::path r_libs_path = r_home_path / "library";
  if (!fs::is_directory(r_libs_path)) {
    throw std::runtime_error("R_LIBS not a directory in R_HOME\n\t-R_LIBS: " +
                             r_libs_path.generic_string());
  }

  if (setenv("R_LIBS", r_libs_path.c_str(), 1) != 0) {
    throw std::runtime_error("setenv() failed on set R_LIBS");
  }
}

// need to set the env var R_HOME to the correct local path based on PWD
// assumption: R installed to r-install/ relative folder to PWD
// assumption: R_HOME can be set to r-install/lib/R/
// should throw if R isn't there so we need a way to check
void set_r_home(const std::filesystem::path &path_to_rhome) {
  namespace fs = std::filesystem;

  if (path_to_rhome.is_absolute()) {
    // assume path provided is the full/complete path to rhome

    check_and_set_rhome(path_to_rhome);

  } else {
    // assume path provided is relative to current path

    // fs::path assumed_rel_path = "r-install/lib64/R/";
    const fs::path &assumed_rel_path = path_to_rhome;
    fs::path current_path = fs::current_path();
    fs::path r_home_path = current_path / assumed_rel_path;

    std::cout << "current path: " << current_path << std::endl;
    std::cout << "desired r_home path: " << r_home_path << std::endl;

    check_and_set_rhome(r_home_path);
  }
  return;
}
