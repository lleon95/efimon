/**
 * @file efimon-meter.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Tool for metering the consumption of existing processes
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <unistd.h>

#include <iostream>

#include <efimon/arg-parser.hpp>
#include <efimon/observer-enums.hpp>
#include <efimon/proc/stat.hpp>
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

  std::cout << "--- Creating process tree ---" << std::endl;
  for (const auto &elem : ptree.GetTree()) {
    std::cout << "\t" << elem << std::endl;
  }

  std::cout << "--- Reading metrics every second for 15 seconds ---"
            << std::endl;
  efimon::ProcStatObserver pstat{(uint)pid, efimon::ObserverScope::PROCESS, 1};
  for (int i = 0; i <= 15; ++i) {
    pstat.Trigger();
    efimon::CPUReadings *readingcpu =
        dynamic_cast<efimon::CPUReadings *>(pstat.GetReadings()[0]);
    efimon::RAMReadings *readingram =
        dynamic_cast<efimon::RAMReadings *>(pstat.GetReadings()[1]);
    std::cout << "\tCPU usage: " << readingcpu->overall_usage << "%, ";
    std::cout << "RAM usage: " << readingram->overall_usage << " MiB"
              << std::endl;
    sleep(1);
  }

  return 0;
}
