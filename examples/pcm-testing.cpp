/**
 * @file pcm-testing
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Proof of Concept Snippet for Intel PCM
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <efimon/power/intel.hpp>
#include <efimon/readings/cpu-readings.hpp>
#include <iostream>

using namespace efimon;  // NOLINT

int main() {
  std::cout << "Hello from PCM" << std::endl;
  std::cout << "INFO: Getting Instance" << std::endl;
  IntelMeterObserver observer{0, ObserverScope::SYSTEM, 0};

  while (true) {
    /* Trigger to get results */
    observer.Trigger();
    auto status = observer.GetStatus();
    if (Status::OK != status.code) {
      std::cerr << "ERROR: The status of the observer is not OK - "
                << status.msg;
      break;
    }
    auto readings_vec = observer.GetReadings();
    for (auto &reading_iface : readings_vec) {
      CPUReadings *readings = dynamic_cast<CPUReadings *>(reading_iface);
      std::cout << "-----------------------------------------------------------"
                   "-----\n"
                << "\tOverall Use: " << readings->overall_usage << " IPC"
                << std::endl
                << "\tOverall Energy: " << readings->overall_energy << " Joules"
                << std::endl
                << "\tOverall Power: " << readings->overall_power << " Watts"
                << std::endl;
      std::cout << "\tCore Usage: ";
      for (auto val : readings->core_usage) {
        std::cout << val << " ";
      }
      std::cout << " IPC" << std::endl << "\tSocket Usage: ";
      for (auto val : readings->socket_usage) {
        std::cout << val << " ";
      }
      std::cout << " IPC" << std::endl << "\tSocket Power: ";
      for (auto val : readings->socket_power) {
        std::cout << val << " ";
      }
      std::cout << " Watts" << std::endl;
      std::cout << " IPC" << std::endl << "\tSocket Energy: ";
      for (auto val : readings->socket_energy) {
        std::cout << val << " ";
      }
      std::cout << " Joules" << std::endl;
    }

    /* Loop every second */
    sleep(1);
  }

  std::cout << "Exiting..." << std::endl;
  return 0;
}
