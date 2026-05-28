/**
 * @file sampler_lib.hpp
 * @author Diego Avila <diego.avila@uned.cr>
 *         Anthony Montero <anthonymr2010@estudiantec.cr>
 * @brief CPU sampling library interface using eBPF
 *
 * @copyright Copyright (c) 2026. See License for Licensing
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

/**
 * @brief Runs eBPF perf-event sampling and appends collected samples to
 * v_out_samples.
 *
 * @param st_config Configuration for the sampling session
 * @param v_out_samples Vector to store collected samples
 * @param str_error_message String to store error messages in case of failure
 * @return true on success, false on failure
 */
bool run_sampling(const Config& st_config, std::vector<Sample>& v_out_samples,
                  std::string& str_error_message);

}  // namespace cpu_sampler
