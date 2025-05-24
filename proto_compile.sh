#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <proto_filename>"
  exit 1
fi

PROTO_FILE=$1

build/_deps/grpc-build/third_party/protobuf/protoc \
  --proto_path=src/proto/ \
  --proto_path=build/_deps/grpc-src/src/proto/ \
  --proto_path=build/_deps/grpc-src/third_party/protobuf/src/ \
  --cpp_out=src/cpp_generated/ \
  --grpc_out=src/cpp_generated/ \
  --plugin=protoc-gen-grpc=build/_deps/grpc-build/grpc_cpp_plugin \
  src/proto/$PROTO_FILE

