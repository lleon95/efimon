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
  /** Average power of all the GPU */
  float overall_power;
  /** Average energy of all the GPU */
  float overall_energy;
  /** Usage per GPU */
  std::vector<float> gpu_usage;
  /** Power per GPU */
  std::vector<float> gpu_power;
  /** Energy per GPU */
  std::vector<float> gpu_energy;
  /** Destructor to enable the inheritance */
  virtual ~GPUReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_GPU_READINGS_HPP_ */
