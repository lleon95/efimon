/**
 * @file perf-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of the use of perf
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <cstdlib>
#include <efimon/perf/annotate.hpp>
#include <efimon/perf/record-readings.hpp>
#include <efimon/perf/record.hpp>
#include <iostream>
#include <string>

using namespace efimon;  // NOLINT

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Need more arguments" << std::endl;
  }

  uint pid = std::atoi(argv[1]);
  PerfRecordObserver record{pid, ObserverScope::PROCESS, 5, 1000, true};
  PerfAnnotateObserver annotate{record};
  record.Trigger();
  annotate.Trigger();
  auto readings = dynamic_cast<RecordReadings *>(record.GetReadings()[0]);

  std::cout << "Results saved in: " << readings->perf_data_path << std::endl;
  std::cout << "Timestamp: " << readings->timestamp << std::endl;
  std::cout << "Difference: " << readings->difference << std::endl;
  return 0;
}
