/**
 * @file cpu-readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings (CPU)
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_READINGS_CPU_READINGS_HPP_
#define INCLUDE_EFIMON_READINGS_CPU_READINGS_HPP_

#include <cstdint>
#include <vector>

#include <efimon/readings.hpp>

namespace efimon {

/**
 * @brief Readings specific to CPU measurements
 */
struct CPUReadings : public Readings {
  /** Average usage of all the cores */
  float overall_usage;
  /** Average power of all the cores */
  float overall_power;
  /** Usage per core */
  std::vector<float> core_usage;
  /** Power per core */
  std::vector<float> core_power;
  /** Destructor to enable the inheritance */
  virtual ~CPUReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_CPU_READINGS_HPP_ */
