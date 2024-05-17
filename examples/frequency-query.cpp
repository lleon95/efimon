/**
 * @file frequency-query.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of efimon::CPUInfo usage
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/proc/cpuinfo.hpp>

#include <unistd.h>

#include <iostream>
#include <utility>

int main(int, char **) {
  efimon::CPUInfo info{};

  std::cout << "Num Sockets: " << info.GetNumSockets()
            << "\nNum Logical Cores: " << info.GetLogicalCores() << std::endl;

  for (uint i = 0; i < 10; ++i) {
    info.Refresh();

    std::vector<float> vector_socket = info.GetSocketMeanFrequency();
    efimon::CPUInfo::CPUAssignment topology = info.GetAssignation();

    std::cout << "Mean Socket Frequency: " << std::endl;

    for (int sock = 0; sock < info.GetNumSockets(); ++sock) {
      std::cout << "\t" << vector_socket[sock] << " MHz" << std::endl;
      for (const auto &tuple : topology[sock]) {
        std::cout << "\t\t" << std::get<0>(tuple) << ": " << std::get<2>(tuple)
                  << " MHz" << std::endl;
      }
    }

    std::cout << "Mean System Frequency: " << info.GetMeanFrequency() << " MHz"
              << std::endl;
    // Wait for 1 second
    sleep(1);
  }

  return 0;
}
