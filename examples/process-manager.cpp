/**
 * @file process-manager.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of process manager launching
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/arg-parser.hpp>
#include <efimon/process-manager.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  using namespace efimon;  // NOLINT

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

  // Prepare the command
  std::string cmd = *bit;
  std::vector<std::string> args(count);
  std::copy(bit, eit, args.begin());

  // Create the process and launch it
  ProcessManager proc;
  Status st;
  if (count == 1) {
    st = proc.Open(cmd);
  } else {
    st = proc.Open(cmd, args);
  }
  if (Status::OK != st.code) {
    std::cerr << "Error: " << st.msg << std::endl;
    return -1;
  }
  std::cout << "\tPID: " << proc.GetPID() << std::endl;
  proc.Sync();

  return 0;
}
