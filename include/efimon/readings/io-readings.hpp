/**
 * @file io-readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings (IO)
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_READINGS_IO_READINGS_HPP_
#define INCLUDE_EFIMON_READINGS_IO_READINGS_HPP_

#include <cstdint>
#include <vector>

#include <efimon/readings.hpp>

namespace efimon {

/**
 * @brief Readings specific to IO measurements
 */
struct IOReadings : public Readings {
  /** Data read bandwidth in kiB/s */
  float read_bw;
  /** Data write bandwidth in kiB/s */
  float write_bw;
  /** Data read volume in kiB */
  uint64_t read_volume;
  /** Data write volume in kiB */
  uint64_t write_volume;
  /** Data read power in Watts */
  float read_power;
  /** Data write power in Watts */
  float write_power;
  virtual ~IOReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_IO_READINGS_HPP_ */
