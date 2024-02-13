/**
 * @file perf-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of the use of perf
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <cstdlib>
#include <iostream>
#include <string>

#include <efimon/perf/record.hpp>

using namespace efimon;  // NOLINT

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Need more arguments" << std::endl;
  }

  uint pid = std::atoi(argv[1]);
  PerfRecordObserver record{pid, ObserverScope::PROCESS, 30, 1000, true};
  record.Trigger();
  return 0;
}
