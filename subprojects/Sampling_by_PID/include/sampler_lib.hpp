/**
 * @file sampler_lib.hpp
 * @author CPU Sampling Module Contributors
 * @brief CPU sampling library interface using eBPF
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cpu_sampler {

struct Sample {
  uint32_t pid;
  uint32_t tid;
  uint64_t ip;
  uint64_t ts;
};

struct Config {
  int target_pid;
  uint64_t frequency_hz;
  int duration_seconds;
};

// Request a graceful stop for a currently running sampling session.
void request_stop();

// Runs eBPF perf-event sampling and appends collected samples to out_samples.
// Returns true on success. If false, error_message contains a human-readable
// error.
bool run_sampling(const Config& config, std::vector<Sample>& out_samples,
                  std::string& error_message);

}  // namespace cpu_sampler
