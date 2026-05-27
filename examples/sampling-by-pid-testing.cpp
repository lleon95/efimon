/**
 * @file sampling-by-pid-testing.cpp
 * @author Diego Avila (diego.avila@uned.cr)
 * @brief Example of eBPF Sampling-by-PID observer testing
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <efimon/ebpf-modules/cpu-assembly-sampler/sampling-by-pid.hpp>
#include <iostream>
#include <string>

using namespace efimon;  // NOLINT

int main(int argc, char **argv) {
  if (argc <= 1) {
    std::cerr << "No PID specified" << std::endl;
    return -1;
  }

  uint u_pid = std::atoi(argv[1]);
  std::cout << "PID: " << u_pid << std::endl;

  SamplingByPIDObserver ob_sampling{u_pid};
  ob_sampling.SetInterval(2000);

  std::cout << "Sampling for 2 seconds..." << std::endl;
  auto st_ret = ob_sampling.Trigger();
  if (st_ret.code != Status::OK) {
    std::cerr << "ERROR: " << st_ret.msg << std::endl;
    return -1;
  }

  std::cout << "Raw eBPF samples collected: "
            << ob_sampling.GetCollectedSamplesCount() << std::endl;
  std::cout << "Decoded userspace samples: "
            << ob_sampling.GetDecodedSamplesCount() << std::endl;

  auto *p_readings_ann =
      dynamic_cast<InstructionReadings *>(ob_sampling.GetReadings()[0]);

  std::cout << "Histogram:" << std::endl;
  for (const auto &kv_histogram : p_readings_ann->histogram) {
    std::cout << "\t" << std::get<0>(kv_histogram) << ": "
              << std::get<1>(kv_histogram) << std::endl;
  }

  std::cout << "Classification:" << std::endl;
  for (const auto &kv_type : p_readings_ann->classification) {
    std::cout << "\t" << AsmClassifier::TypeString(kv_type.first) << ": "
              << std::endl;
    for (const auto &kv_family : kv_type.second) {
      std::cout << "\t\t" << AsmClassifier::FamilyString(kv_family.first)
                << ": " << std::endl;
      for (const auto &kv_origin : kv_family.second) {
        std::cout << "\t\t\t" << AsmClassifier::OriginString(kv_origin.first)
                  << ": " << kv_origin.second << std::endl;
      }
    }
  }

  st_ret = ob_sampling.Trigger();

  return 0;
}
