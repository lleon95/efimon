/**
 * @file process-manager-threaded.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of process manager launching with multiple threads
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/arg-parser.hpp>
#include <efimon/process-manager.hpp>

#include <algorithm>
#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <iostream>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT
#include <vector>

using namespace efimon;  // NOLINT

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
    return;
  }

  proc.Sync();
}

int main(int argc, char **argv) {
  // Check arguments
  ArgParser argparser(argc, argv);
  if (argc < 3 || !(argparser.Exists("-c"))) {
    std::cerr << "Error: Wrong usage" << std::endl
              << "\tUsage: " << argv[0] << " -c [COMMAND]" << std::endl;
    return -1;
  }
  std::cout << "Executing:" << std::endl;

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
  std::vector<std::string> args_user(count);
  std::copy(bit, eit, args_user.begin());
  // Prepare the test command
  std::vector<std::string> args_sleep = {"time", "sleep", "10"};

  // Prepare the sync mechanisms
  std::mutex m1, m2;
  std::condition_variable cv1, cv2;

  // Launch in two different threads
  ProcessManager proc1, proc2;
  std::thread t1(launch_command, std::ref(proc1), std::ref(args_user),
                 std::ref(cv1));
  std::thread t2(launch_command, std::ref(proc2), std::ref(args_sleep),
                 std::ref(cv2));

  // Print the process PID
  {
    std::unique_lock lk(m1);
    cv1.wait_for(lk, std::chrono::seconds(1));
    std::cout << "\tPID User Command: " << proc1.GetPID() << std::endl;
  }
  {
    std::unique_lock lk(m2);
    cv2.wait_for(lk, std::chrono::seconds(1));
    std::cout << "\tPID Sleep Command: " << proc2.GetPID() << std::endl;
  }

  // Synchronise
  t1.join();
  t2.join();

  return 0;
}
