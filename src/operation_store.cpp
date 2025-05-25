#include "operation_store.h"

EvalOperation EvalOperationStore::createEvalOperation(const std::string& name_uuid) {
  // ensure locked
  std::lock_guard<std::mutex> lock(store_mutex_);

  EvalOperationData new_data;
  new_data.operation_proto.set_name(name_uuid);
  new_data.operation_proto.set_done(false);
  // leave others empty (result and duration)

  auto now = std::chrono::system_clock::now();
  new_data.creation_time = now;

  operations_[name_uuid] = new_data;
  return new_data.operation_proto;
}

std::optional<EvalOperation> EvalOperationStore::getEvalOperation(const std::string& name_uuid) const {
  std::lock_guard<std::mutex> lock(store_mutex_);

  auto it = operations_.find(name_uuid);
  // if (item_found)
  if (it != operations_.end()) {
    // return the eval operation
    return it->second.operation_proto;
  }

  return std::nullopt;
}

bool EvalOperationStore::updateEvalOperation(const std::string& name_uuid,
                                            const std::function<void(EvalOperation& eval_operation_proto)>& updater) {
  std::lock_guard<std::mutex> lock(store_mutex_);
  auto it = operations_.find(name_uuid);
  if (it != operations_.end()) {
    updater(it->second.operation_proto);
    return true;
  }

  return false;
}
