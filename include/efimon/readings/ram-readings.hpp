/**
 * @file ram-readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings (RAM)
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_READINGS_RAM_READINGS_HPP_
#define INCLUDE_EFIMON_READINGS_RAM_READINGS_HPP_

#include <cstdint>
#include <vector>

#include <efimon/readings.hpp>

namespace efimon {

/**
 * @brief Readings specific to RAM measurements
 */
struct RAMReadings : public Readings {
  /** Overall RAM usage in MiB */
  float overall_usage;
  /** Total memory usage: RAM + SWAP in MiB */
  float total_memory_usage;
  /** Overall SWAP memory usage in MiB */
  float swap_usage;
  /** Overall Bandwidth in MiB/s */
  float overall_bw;
  /** Overall power consumed by the RAM in Watts */
  float overall_power;
  /** Destructor to enable the inheritance */
  virtual ~RAMReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_RAM_READINGS_HPP_ */
