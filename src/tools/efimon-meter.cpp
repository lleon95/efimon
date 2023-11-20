/**
 * @file efimon-meter.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Tool for metering the consumption of existing processes
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <iostream>

#include <efimon/arg-parser.hpp>
#include <efimon/proc/thread-tree.hpp>

int main(int argc, char **argv) {
  auto cli = efimon::ArgParser{argc, argv};

  if (!cli.Exists("--pid")) {
    std::cerr << "--pid option not given and it is mandatory" << std::endl;
    return -1;
  }

  int pid = std::stoi(cli.GetOption("--pid"));

  std::cout << "Program name: " << cli.GetOption("--program-name") << std::endl;
  std::cout << "PID: " << pid << std::endl;

  efimon::ThreadTree ptree{pid};

  for (const auto &elem : ptree.GetTree()) {
    std::cout << "\t" << elem << std::endl;
  }

  return 0;
}
