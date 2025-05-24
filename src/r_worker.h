#pragma once

#include "r_result.h"
#include "r_task.h"
#include <concurrentqueue.h>
#include <memory>

namespace RWorker {
using namespace moodycamel;
void r_worker_thread(
    ConcurrentQueue<std::unique_ptr<RTask>> &taskQueue,
    ConcurrentQueue<std::unique_ptr<RResponse>> &responseQueue);

extern bool is_R_init;
} // namespace RWorker
