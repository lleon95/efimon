/**
 * @file ptrace-capstone-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of RAPL testing
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <efimon/ptrace-capstone/ptrace-capstone.hpp>
#include <iostream>
#include <string>

using namespace efimon;  // NOLINT

int main(int argc, char **argv) {
  if (argc <= 1) {
    std::cerr << "No PID specified" << std::endl;
    return -1;
  }

  uint pid = std::atoi(argv[1]);
  std::cout << "PID: " << pid << std::endl;

  PTraceCapstoneObserver observer{pid};
  observer.SetInterval(1000);

  auto ret = observer.Trigger();
  if (ret.code != Status::OK) {
    std::cerr << "ERROR: " << ret.msg << std::endl;
    return -1;
  }

  auto readings_ann =
      dynamic_cast<InstructionReadings *>(observer.GetReadings()[0]);

  std::cout << "Histogram:" << std::endl;
  for (const auto &pair : readings_ann->histogram) {
    std::cout << "\t" << std::get<0>(pair) << ": " << std::get<1>(pair)
              << std::endl;
  }

  std::cout << "Classification:" << std::endl;
  for (const auto &type : readings_ann->classification) {
    std::cout << "\t" << AsmClassifier::TypeString(type.first) << ": "
              << std::endl;
    for (const auto &family : type.second) {
      std::cout << "\t\t" << AsmClassifier::FamilyString(family.first) << ": "
                << std::endl;
      for (const auto &origin : family.second) {
        std::cout << "\t\t\t" << AsmClassifier::OriginString(origin.first)
                  << ": " << origin.second << std::endl;
      }
    }
  }

  ret = observer.Trigger();

  return 0;
}
