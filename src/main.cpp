// local project
#include "concurrentqueue.h"
#include "r_result.h"
#include "r_task.h"
#include "r_worker.h"
#include <iostream>
#include <memory>

int main() {
  // namespace for ConcurrentQueue
  using namespace moodycamel;
  using namespace RWorker;

  ConcurrentQueue<std::unique_ptr<RTask>> taskQueue;
  ConcurrentQueue<std::unique_ptr<RResponse>> responseQueue;

  std::unique_ptr<RTask> sample_R_task =
      RTask::create_client_r_code_task("print(\"hello\")\nplot(c(1,2,3,4,5))\nclint_func()\nwarning(\"hello?\")");
  std::unique_ptr<RTask> sample_echo_task =
      RTask::create_cpp_management_task("echo", {"hello"});

  // taskQueue.enqueue(sample_R_task);

  taskQueue.enqueue(std::move(sample_R_task));
  taskQueue.enqueue(std::move(sample_echo_task));
  // create thread
  std::thread rWorkerThread(RWorker::r_worker_thread, std::ref(taskQueue),
                            std::ref(responseQueue));

  rWorkerThread.join();

  std::unique_ptr<RResponse> r_response;
  while (responseQueue.try_dequeue(r_response)) {
    std::cout << *r_response << std::endl;
  }

  return 0;
}
