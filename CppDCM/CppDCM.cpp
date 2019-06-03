#include "DCM_Client.h"
#include "DCM_TaskRunner.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <iostream>

void init_loggers() {
  auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();

  auto core_logger = std::make_shared<spdlog::logger>("core", sink);
  spdlog::register_logger(core_logger);
  spdlog::set_default_logger(core_logger);

  auto client_logger = std::make_shared<spdlog::logger>("client", sink);
  spdlog::register_logger(client_logger);

  auto runner_logger = std::make_shared<spdlog::logger>("runner", sink);
  spdlog::register_logger(runner_logger);

  spdlog::set_level(spdlog::level::debug);
}

int main(int argc, char *argv[]) {
  DCM::TaskRunner<3, 3> task_runner;

  init_loggers();

  spdlog::info("n: {} fn: {} nm: {} maxW: {} nmbits: {} maxInt: {} maxIntM: {} noc: {} nocThresh: {}",
               task_runner.n, task_runner.fn,
               task_runner.nm, task_runner.maxW,
               task_runner.nmbits, task_runner.maxInt,
               task_runner.maxIntM, task_runner.noc,
               task_runner.nocThresh);

  DCM::Client client("http://127.0.0.1/ChaffinMethod.php", "13", "The A Team");

  client.registerClient();

  DCM::Task task = client.getTask();

  task_runner.runTask(task, &client);

  exit(0);
}
