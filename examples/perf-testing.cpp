/**
 * @file perf-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of the use of perf
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <cstdlib>
#include <efimon/asm-classifier.hpp>
#include <efimon/perf/annotate.hpp>
#include <efimon/perf/record-readings.hpp>
#include <efimon/perf/record.hpp>
#include <efimon/readings/instruction-readings.hpp>
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
  auto readings_rec = dynamic_cast<RecordReadings *>(record.GetReadings()[0]);
  auto readings_ann =
      dynamic_cast<InstructionReadings *>(annotate.GetReadings()[0]);

  std::cout << "Record: Results saved in: " << readings_rec->perf_data_path
            << std::endl;
  std::cout << "Record: Timestamp: " << readings_rec->timestamp << std::endl;
  std::cout << "Record: Difference: " << readings_rec->difference << std::endl;

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
                << family.second << std::endl;
    }
  }

  std::cout << "Annotate: Timestamp: " << readings_ann->timestamp << std::endl;
  std::cout << "Annotate: Difference: " << readings_ann->difference
            << std::endl;
  return 0;
}
