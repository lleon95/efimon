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

  for (uint i = 0; i < 30; ++i) {
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
  }

  return 0;
}
