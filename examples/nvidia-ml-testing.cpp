/**
 * @file rapl-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of RAPL testing
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <efimon/gpu/nvidia.hpp>
#include <iostream>
#include <string>

using namespace efimon;  // NOLINT

static constexpr int kDelay = 1;  // 1 second

int main(int argc, char **argv) {
  uint pid = 0;
  ObserverScope scope;

  if (argc >= 2) {
    pid = std::atoi(argv[1]);
  }

  if (0 == pid) {
    scope = ObserverScope::SYSTEM;
    std::cout << "Querying System Metrics" << std::endl;
  } else {
    scope = ObserverScope::PROCESS;
    std::cout << "Querying Process Metrics, PID: " << pid << std::endl;
  }

  NVIDIAMeterObserver meter{pid, scope};
  auto readings_iface = meter.GetReadings()[0];
  GPUReadings *readings = dynamic_cast<GPUReadings *>(readings_iface);

  for (uint i = 0; i < 30; ++i) {
    sleep(kDelay);
    auto res = meter.Trigger();
    if (Status::OK != res.code) {
      std::cerr << "ERROR: " << res.what() << std::endl;
      break;
    }

    std::cout << "Overall Usage: " << readings->overall_usage << " %"
              << std::endl;

    if (ObserverScope::SYSTEM == scope) {
      std::cout << "Overall Memory: " << readings->overall_memory << " %"
                << std::endl;
      std::cout << "Overall Power: " << readings->overall_power << " W"
                << std::endl;
      std::cout << "Overall Clock SM: " << readings->clock_speed_sm << " MHz"
                << std::endl;
      std::cout << "Overall Clock MEM: " << readings->clock_speed_mem << " MHz"
                << std::endl;
    } else {
      std::cout << "Overall Memory: " << readings->overall_memory << " kiB"
                << std::endl;
    }
  }

  return 0;
}
