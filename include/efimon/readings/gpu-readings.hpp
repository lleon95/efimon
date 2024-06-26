/**
 * @file gpu-readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings (GPU)
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_READINGS_GPU_READINGS_HPP_
#define INCLUDE_EFIMON_READINGS_GPU_READINGS_HPP_

#include <cstdint>
#include <efimon/readings.hpp>
#include <vector>

namespace efimon {

/**
 * @brief Readings specific to GPU measurements
 */
struct GPUReadings : public Readings {
  /** Average usage of all the GPU */
  float overall_usage;
  /** Average power of all the GPU in Watts. Available in SYSTEM */
  float overall_power;
  /** Average memory of all the GPU in KiB. When SYSTEM, it is a percentage */
  float overall_memory;
  /** Average SM clock speed in MHz. Available in SYSTEM */
  float clock_speed_sm;
  /** Average MEM clock speed in MHz. Available in SYSTEM */
  float clock_speed_mem;
  /** Usage per GPU: Currently unused */
  std::vector<float> gpu_usage;
  /** Power per GPU: Currently unused */
  std::vector<float> gpu_power;
  /** Energy per GPU: Currently unused */
  std::vector<float> gpu_energy;
  /** Destructor to enable the inheritance */
  virtual ~GPUReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_GPU_READINGS_HPP_ */
