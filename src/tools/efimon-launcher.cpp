/**
 * @file efimon-launcher.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Wrapper tool for metering the consumption of existing processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <json/json.h>

#include <atomic>
#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <efimon/arg-parser.hpp>
#include <efimon/logger/macros.hpp>
#include <efimon/proc/cpuinfo.hpp>
#include <efimon/process-manager.hpp>
#include <efimon/status.hpp>
#include <memory>
#include <mutex>  // NOLINT
#include <sstream>
#include <string>
#include <thread>  // NOLINT
#include <zmq.hpp>

#include "macro-handling.hpp"  // NOLINT

static constexpr int kThreadCheckTime = 10;   // 10 millis
static constexpr int kThreadStartUpTime = 3;  // 3 seconds

using namespace efimon;  // NOLINT

struct AppData {
  // Manages the app arguments and data
  std::vector<std::string> command;
  uint port;
  uint pid;
  uint frequency;
  uint samples;
  uint delay;

  // Manages the process manager
  ProcessManager manager;
  std::mutex manager_mtx;
  std::condition_variable manager_cv;
  std::thread manager_th;
  std::atomic<bool> close;
  std::atomic<bool> terminated;
};

void print_welcome() {
  std::cout << "-----------------------------------------------------------\n";
  std::cout << "               EfiMon Launcher Application                 \n";
  std::cout << "-----------------------------------------------------------\n";
}

std::string get_help(char **argv) {
  std::string msg =
      "This application launches a daemon wrapper for measuring external "
      "applications: EfiMon Launcher\n\tUsage: "
      "\n\t";
  msg += std::string(argv[0]);
  msg +=
      " -s,--samples SAMPLES (default: 100). Number of samples to "
      "collect\n\t\t";
  msg +=
      " -f,--frequency FREQUENCY_HZ (default: 100 Hz). Sampling "
      "frequency\n\t\t";
  msg += " -d,--delay DELAY_SECS (default: 3 Secs). Sampling time window\n\t\t";
  msg +=
      " -c,--command COMMAND. Command to execute. This option must be at "
      "the end of the launcher command\n\t\t";
  msg +=
      " -pid,--pid PID. PID to attach to. This option must be at "
      "the end of the launcher command\n\t\t";
  msg +=
      " -p,--port PORT (default: 5550 Secs). EfiMon Socket Port for "
      "IPC\n\t\t";
  msg += " -h,--help: prints this message\n\n";
  msg +=
      " \tBy default, the outputs will be saved into the folder with the "
      "pattern output-pid.csv\n";
  return msg;
}

void launch_command(AppData &data) {  // NOLINT
  // Create the process and launch it
  Status st;
  uint count = data.command.size();
  data.manager_mtx.lock();
  if (count == 1) {
    st = data.manager.Open(data.command[0], ProcessManager::Mode::SILENT);
  } else {
    st = data.manager.Open(data.command[0], data.command,
                           ProcessManager::Mode::SILENT);
  }
  data.manager_mtx.unlock();

  if (Status::OK != st.code) {
    data.manager_mtx.lock();
    data.terminated.store(true);
    data.manager_mtx.unlock();
    data.manager_cv.notify_one();
    return;
  }
  data.manager_cv.notify_one();

  bool local_close = false;
  while (!local_close) {
    if (!data.manager.IsRunning()) {
      data.manager_mtx.lock();
      data.terminated.store(true);
      data.manager_mtx.unlock();
      break;
    }

    // Sleep while running
    std::this_thread::sleep_for(std::chrono::milliseconds(kThreadCheckTime));

    // Check close status
    data.manager_mtx.lock();
    local_close = data.close.load();
    data.manager_mtx.unlock();
  }
  data.manager_mtx.lock();
  data.manager.Close();
  data.manager_mtx.unlock();
}

int main(int argc, char **argv) {
  print_welcome();

  // ------------ Application data -------------------
  AppData appdata;

  // ------------ Arguments ------------
  ArgParser argparser(argc, argv);

  bool check_frequency =
      argparser.Exists("-f") || argparser.Exists("--frequency");
  bool check_samples = argparser.Exists("-s") || argparser.Exists("--samples");
  bool check_delay = argparser.Exists("-d") || argparser.Exists("--delay");
  bool check_help = argparser.Exists("-h") || argparser.Exists("--help");
  bool check_port = argparser.Exists("-p") || argparser.Exists("--port");
  bool check_command = argparser.Exists("-c") || argparser.Exists("--command");
  bool check_pid = argparser.Exists("-pid") || argparser.Exists("--pid");

  if (check_help) {
    std::string msg = get_help(argv);
    EFM_ERROR(msg);
  }

  if (!check_command && !check_pid) {
    EFM_ERROR("Cannot execute without a command or a PID");
    std::string msg = get_help(argv);
    EFM_ERROR(msg);
  }

  // ------------ Modify the configurations -----------
  if (check_command) {
    // Get the arguments from the command
    auto bit = argparser.GetBegin("-c");
    auto eit = argparser.GetEnd();
    const int count = eit - bit;
    appdata.command.resize(count);
    std::copy(bit, eit, appdata.command.begin());
  } else {
    appdata.command.clear();
    appdata.pid =
        std::stoi(argparser.Exists("-pid") ? argparser.GetOption("-pid")
                                           : argparser.GetOption("--pid"));
  }

  if (check_samples) {
    appdata.samples =
        std::stoi(argparser.Exists("-s") ? argparser.GetOption("-s")
                                         : argparser.GetOption("--samples"));
  }

  if (check_frequency) {
    appdata.frequency =
        std::stoi(argparser.Exists("-f") ? argparser.GetOption("-f")
                                         : argparser.GetOption("--frequency"));
  }

  if (check_delay) {
    appdata.delay =
        std::stoi(argparser.Exists("-d") ? argparser.GetOption("-d")
                                         : argparser.GetOption("--delay"));
  }

  if (check_port) {
    appdata.port =
        std::stoi(argparser.Exists("-p") ? argparser.GetOption("-p")
                                         : argparser.GetOption("--port"));
  }

  EFM_INFO(std::string("Frequency [Hz]: ") + std::to_string(appdata.frequency));
  EFM_INFO(std::string("Samples: ") + std::to_string(appdata.samples));
  EFM_INFO(std::string("Delay time [secs]: ") + std::to_string(appdata.delay));
  EFM_INFO(std::string("IPC TCP Port: ") + std::to_string(appdata.port));

  // Launch the Process
  if (check_command) {
    EFM_INFO("Launching the process with command: " + appdata.command[0]);
    appdata.manager_th = std::thread(launch_command, std::ref(appdata));
    {
      std::unique_lock lk(appdata.manager_mtx);
      appdata.manager_cv.wait_for(lk, std::chrono::seconds(kThreadStartUpTime));
    }
  } else {
    EFM_INFO("Launching the listener with PID: " + std::to_string(appdata.pid));
  }

  // Join the Process
  return 0;
}
