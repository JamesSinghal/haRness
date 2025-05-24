#
# Top level CMake config for haRness
# - Finds and links against the system-installed version of R
#
# Assumptions:
# - Assumes the file/folder structure *does not* change.
# - Requires R to be installed on the system and findable by CMake (e.g., R executable in PATH).
#

cmake_minimum_required(VERSION 3.15)
project(haRness)
include(FetchContent)

# we need to make our own and it will be cmake/FindR.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Standard CMake includes and settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Find the system R installation
# This will define R_INCLUDE_DIR, R_LIBRARIES, R_LIBRARY_DIRS, R_EXECUTABLE etc.
find_package(R REQUIRED QUIET)

message(STATUS "Found R version: ${R_VERSION_STRING}")
message(STATUS "Using R_INCLUDE_DIR: ${R_INCLUDE_DIR}")
message(STATUS "Using R_LIBRARIES: ${R_LIBRARIES}")
message(STATUS "Using R_LIBRARY_DIRS: ${R_LIBRARY_DIRS}")
message(STATUS "R home directory found by CMake: ${R_HOME_DIR}") # R_HOME_DIR is set by FindR
message(STATUS "R LD FLAGS: ${R_LD_FLAGS}")

# get concurrentqueue library for messaging
# might move to boost.interprocess or zeromq if we move to multiple processes
FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
  GIT_TAG v1.0.4
)

FetchContent_MakeAvailable(concurrentqueue)

# FIXME: consider using a different approach
file(GLOB HARNESS_SOURCES "src/*.cpp")

# Actual haRness executable
add_executable(haRness ${HARNESS_SOURCES})

target_compile_options(haRness PRIVATE -fPIE)
target_compile_options(haRness PRIVATE -march=x86-64)
#target_compile_options(haRness PRIVATE -mavx -mavx2 -march=x86-64)

# set for main.cpp to read
# guarded by ifndef
target_compile_definitions(haRness PRIVATE "R_HOME_CMAKE=\"${R_HOME_DIR}\"")

# Include the R headers for haRness
target_include_directories(haRness PRIVATE
  ${R_INCLUDE_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)


# compile options for haRness
target_compile_options(haRness PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)

separate_arguments(R_LD_FLAGS_LIST UNIX_COMMAND "${R_LD_FLAGS}")

target_link_options(haRness PRIVATE ${R_LD_FLAGS_LIST})

# Link haRness against the R libraries
# R_LIBRARIES should contain all necessary R libraries (e.g., R, Rblas, Rlapack)
target_link_libraries(haRness PRIVATE ${R_LIBRARIES})

# link concurrentqueue
target_link_libraries(haRness PRIVATE concurrentqueue)

FetchContent_Declare(
  rcpp11
  GIT_REPOSITORY https://github.com/r-lib/cpp11.git
  GIT_TAG v0.5.2
)

FetchContent_MakeAvailable(rcpp11)
target_include_directories(haRness PRIVATE
  ${rcpp11_SOURCE_DIR}/inst/include)

message("rcpp source dir: ${rcpp11_SOURCE_DIR}")

set(FETCHCONTENT_QUIET OFF)
set(ABSL_ENABLE_INSTALL ON) 

set(gRPC_BUILD_TESTS OFF)
set(gRPC_BUILD_CSHARP_EXT OFF)
set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF)
set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF)
set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF)
set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF)
set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF)
set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF)
set(ABSL_BUILD_TESTING OFF)
set(protobuf_BUILD_TESTS OFF)
set(protobuf_BUILD_EXAMPLES OFF)


# get gRPC
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG v1.70.2
)
FetchContent_MakeAvailable(gRPC)

target_link_libraries(haRness PRIVATE grpc++)


# grpc and protobuf handling
set(PROTO_FILES "reval_service.proto")
set(GENERATED_PROTOBUF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp_generated")

file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})

# Generate protobuf/gRPC files using your script
foreach(proto_file ${PROTO_FILES})
  get_filename_component(proto_name ${proto_file} NAME_WE)
  set(GENERATED_SOURCES ${GENERATED_SOURCES}
    "${GENERATED_PROTOBUF_PATH}/${proto_name}.pb.cc"
    "${GENERATED_PROTOBUF_PATH}/${proto_name}.grpc.pb.cc"
  )
  add_custom_command(
    OUTPUT "${GENERATED_PROTOBUF_PATH}/${proto_name}.pb.cc"
           "${GENERATED_PROTOBUF_PATH}/${proto_name}.grpc.pb.cc"
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/proto_compile.sh ${proto_file}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/proto/${proto_file}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating protobuf files for ${proto_file}"
  )
endforeach()

# Update the executable to include generated sources
target_sources(haRness PRIVATE ${GENERATED_SOURCES})

# Add generated headers to include path
target_include_directories(haRness PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp_generated"  # Add this line
)



