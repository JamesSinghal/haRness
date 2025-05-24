#include "r_result.h"
#include <memory>

namespace RWorker {
std::unique_ptr<RResponse> eval_client_R(std::string code,
                                         std::string task_uuid);
}
