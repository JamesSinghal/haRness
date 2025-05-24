# CMake module that will be called by the root CMakeLists.txt folder
# to find and set defines for the system installation of R
#
# Future versions of this will possibly be used to support switching to
# non-system versions of R, which would allow the root CMakeLists.txt to avoid
# having to embed that logic.
#
# To accomplish the goal of allowing haRness to embed R, these defines need
# to be set, which will be used by root CMakeLists.txt (and also main.cpp through a compiler define):
# - R_VERSION_STRING
# - R_INCLUDE_DIR
# - R_LIBRARIES
# - R_LIBRARY_DIRS
# - R_HOME_DIR
#
# find_package() is called with REQUIRED, so we need to fail if not found
#
# this is running in "module" mode too

include(FindPackageHandleStandardArgs)

# fail if not on linux
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "haRness can only be built on Linux. Current sys name: ${CMAKE_HOST_SYSTEM_NAME}")
else()
  message(STATUS "On Linux")
endif()

find_program(R_EXECUTABLE NAMES R)

# check if R is found or not
if(NOT R_EXECUTABLE)
  message(FATAL_ERROR "R NOT FOUND")
else()
  message(STATUS "R executable found path: ${R_EXECUTABLE}")
endif()

# if we're here, R is found and the path is in R_EXECUTABLE
# use R commands to figure out the necessary variables
execute_process(COMMAND ${R_EXECUTABLE} RHOME
                  OUTPUT_VARIABLE R_HOME_DIR
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(R_ROOT_DIR "${R_HOME_DIR}")

# find the header folder, frankly should just be in rhome/include
find_path(R_INCLUDE_DIR R.h
          HINTS ${R_HOME_DIR}
          PATHS /usr/lib64/
          PATH_SUFFIXES include)

if(NOT R_INCLUDE_DIR)
  message(FATAL_ERROR "Couldn't find home dir and include path")
else()
  message(STATUS "R_HOME: ${R_HOME_DIR}, R_INCLUDE: ${R_INCLUDE_DIR}")
  set(R_FOUND TRUE)
endif()

# we found R_HOME and include dir correctly.
# now we need the library paths and version
find_library(R_LIBRARIES R
              HINTS ${R_HOME_DIR}/lib
              PATHS /usr/lib64/)

if(NOT R_LIBRARIES)
  message(FATAL_ERROR "Libraries not found")
else()
  message(STATUS "R libraries found: ${R_LIBRARIES}")
endif()

#get lib path
cmake_path(GET R_LIBRARIES PARENT_PATH R_LIBRARY_DIRS)

# TODO: fix this
set(R_VERSION_STRING "4.5.0")

# get ld flags
execute_process(COMMAND ${R_EXECUTABLE} CMD config --ldflags
                  OUTPUT_VARIABLE R_LD_FLAGS
                  )

message(STATUS "R_LIBRARY_DIRS: ${R_LIBRARY_DIRS}, R_LD_FLAGS: ${R_LD_FLAGS}")

mark_as_advanced(R_FOUND R_EXECUTABLE R_ROOT_DIR R_INCLUDE_DIR R_LIBRARIES R_LIBRARY_DIRS R_LD_FLAGS)
