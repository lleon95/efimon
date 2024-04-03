/**
 * @file ipmi-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of RAPL testing
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <efimon/power/ipmi.hpp>
#include <iostream>
#include <string>

using namespace efimon;  // NOLINT

static constexpr int kDelay = 1;  // 1 second

int main(int, char **) {
  IPMIMeterObserver ipmi_meter{};
  auto readings_iface = ipmi_meter.GetReadings()[0];
  PSUReadings *readings = dynamic_cast<PSUReadings *>(readings_iface);

  for (uint i = 0; i < 10; ++i) {
    sleep(kDelay);
    ipmi_meter.Trigger();

    uint psu_num = readings->psu_max_power.size();
    std::cout << "PSU Detected: " << psu_num << std::endl;
    for (uint i = 0; i < psu_num; ++i) {
      std::cout << "\t" << i << ": " << readings->psu_power.at(i) << "  Watts "
                << readings->psu_energy.at(i) << "  Joules" << std::endl;
    }
    std::cout << "Average Power: " << readings->overall_power << " Watts"
              << std::endl;
    std::cout << "Average Energy: " << readings->overall_energy << " Joules"
              << std::endl;
  }

  return 0;
}
