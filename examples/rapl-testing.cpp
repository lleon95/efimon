/**
 * @file rapl-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of RAPL testing
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <efimon/power/rapl.hpp>
#include <efimon/proc/cpuinfo.hpp>
#include <iostream>
#include <string>

using namespace efimon;  // NOLINT

static constexpr int kNumSockets = 10;  // unrealistic for example purposes
static constexpr int kDelay = 1;        // 1 second

int main(int argc, char **argv) {
  uint socketid = kNumSockets;

  if (argc > 2) {
    socketid = std::atoi(argv[1]);
  }

  if (kNumSockets == socketid) {
    std::cout << "Analysing all sockets" << std::endl;
  } else {
    std::cout << "Socket: " << socketid << std::endl;
  }

  RAPLMeterObserver rapl_meter{};
  auto readings_iface = rapl_meter.GetReadings()[0];
  CPUReadings *readings = dynamic_cast<CPUReadings *>(readings_iface);

  for (uint i = 0; i < 10; ++i) {
    sleep(kDelay);
    rapl_meter.Trigger();

    std::cout << "Sockets Detected: " << readings->socket_power.size()
              << std::endl;
    uint cont = 0;
    for (const auto socket_energy : readings->socket_power) {
      std::cout << "\t" << cont << ": " << (socket_energy / kDelay) << "  Watts"
                << std::endl;
    }
    std::cout << "Average Power: " << (readings->overall_power / kDelay)
              << " Watts" << std::endl;
  }

  return 0;
}
