/**
 * @file rapl-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of RAPL testing
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <atomic>              // NOLINT
#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <efimon/arg-parser.hpp>
#include <efimon/gpu/nvidia.hpp>
#include <efimon/process-manager.hpp>
#include <iostream>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT
#include <vector>

using namespace efimon;  // NOLINT

static constexpr int kDelay = 1;  // 1 second

std::atomic<int> running{1};

void launch_command(ProcessManager &proc,                  // NOLINT
                    const std::vector<std::string> &args,  // NOLINT
                    std::condition_variable &cv) {         // NOLINT
  // Create the process and launch it
  Status st;
  uint count = args.size();

  if (count == 1) {
    st = proc.Open(args[0]);
  } else {
    st = proc.Open(args[0], args);
  }

  cv.notify_one();

  if (Status::OK != st.code) {
    running.store(0);
    return;
  }

  proc.Sync();
  running.store(0);
}

int main(int argc, char **argv) {
  uint pid = 0;
  ObserverScope scope = ObserverScope::SYSTEM;

  ArgParser argparser(argc, argv);
  if (argc < 3 || !(argparser.Exists("-c"))) {
    std::cerr << "Error: Wrong usage" << std::endl
              << "\tUsage: " << argv[0] << " -c [COMMAND]" << std::endl;
    return -1;
  }

  // Print the argument
  auto bit = argparser.GetBegin("-c");
  auto eit = argparser.GetEnd();
  const int count = eit - bit;
  std::cout << "\tTotal args: " << count << "\n\t";
  for (auto it = bit; it != eit; it++) {
    std::cout << *it << " ";
  }
  std::cout << std::endl;

  // Prepare the user command
  std::vector<std::string> args_user(count + 6);
  args_user[0] = "nvprof";
  args_user[1] = "--print-gpu-trace";
  args_user[2] = "-f";
  args_user[3] = "--csv";
  args_user[4] = "--log-file";
  args_user[5] = "nvprof-log.log";
  std::copy(bit, eit, args_user.begin() + 6);

  std::cout << "\t";
  for (auto &i : args_user) {
    std::cout << i << " ";
  }
  std::cout << std::endl;

  // Prepare the sync mechanisms
  std::mutex m1;
  std::condition_variable cv1;

  // Launch Process
  ProcessManager proc1;
  std::thread t1(launch_command, std::ref(proc1), std::ref(args_user),
                 std::ref(cv1));

  // Print the process PID
  {
    std::unique_lock lk(m1);
    cv1.wait_for(lk, std::chrono::seconds(1));
    std::cout << "\tPID User Command: " << proc1.GetPID() << std::endl;
  }

  NVIDIAMeterObserver meter{pid, scope};
  auto readings_iface = meter.GetReadings()[0];
  GPUReadings *readings = dynamic_cast<GPUReadings *>(readings_iface);

  // Create table
  if (ObserverScope::SYSTEM == scope) {
    std::cout << "OverallUsage(perc),"
              << "OverallMemory(perc),"
              << "OverallPower(W),"
              << "ClockSM(MHz),"
              << "ClockMEM(MHz)" << std::endl;
  } else {
    std::cout << "OverallUsage(perc),OverallMemory(kiB)" << std::endl;
  }

  for (uint i = 0; i < 50; ++i) {
    sleep(kDelay);
    auto res = meter.Trigger();
    if (Status::OK != res.code) {
      std::cerr << "ERROR: " << res.what() << std::endl;
      break;
    }

    std::cout << readings->overall_usage << ",";

    if (ObserverScope::SYSTEM == scope) {
      std::cout << readings->overall_memory << ",";
      std::cout << readings->overall_power << ",";
      std::cout << readings->clock_speed_sm << ",";
      std::cout << readings->clock_speed_mem << std::endl;
    } else {
      std::cout << readings->overall_memory << std::endl;
    }

    if (running.load() == 0) break;
  }

  // Synchronise
  t1.join();

  return 0;
}
