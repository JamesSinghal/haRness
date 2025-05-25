#pragma once

#include "reval_service.pb.h"
#include <chrono>
#include <string>
#include <optional>
#include <functional>
#include "absl/container/flat_hash_map.h"
#include <mutex>

// eval operation data to be stored in the map
struct EvalOperationData {
  EvalOperation operation_proto;
  std::chrono::system_clock::time_point creation_time;

  // more later  
};

class EvalOperationStore {
public:
  EvalOperationStore() = default;

  // remove the ability to copy or move to prevent any accidental
  // errors. it should be a singleton or essentially unmoving
  EvalOperationStore(const EvalOperationStore&) = delete;
  EvalOperationStore& operator=(const EvalOperationStore&) = delete;

  // create an operation and store initial state, providing the same
  // uuid from the RTask that is constructed
  EvalOperation createEvalOperation(const std::string& name_uuid);

  // get the operation pointed to by the uuid/name requested
  // returns an optional which would be empty if operation name not found
  std::optional<EvalOperation> getEvalOperation(const std::string& name_uuid) const;

  // updates an EvalOperation in the map by allowing the passing of an
  // updater function that receives a mutable reference to the EvalOperation
  bool updateEvalOperation(const std::string& operation_name,
                           const std::function<void(EvalOperation& eval_operation_proto)>& updater);

  // later can consider having a function that removes operations that are too old
  // based on a set maximum age; probably not important for now.
  

private:
  mutable std::mutex store_mutex_;
  absl::flat_hash_map<std::string, EvalOperationData> operations_ ABSL_GUARDED_BY(store_mutex_);
};
