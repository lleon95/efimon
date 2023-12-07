/**
 * @file readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_READINGS_HPP_
#define INCLUDE_EFIMON_READINGS_HPP_

#include <cstdint>

namespace efimon {

/**
 * @brief Fundamental structure that spawn readings
 *
 * This works as a base for further readers
 */
struct Readings {
  /** Abstract representation of the observer type */
  uint64_t type;
  /** Current timestamp at the time of the measurement in ms */
  uint64_t timestamp;
  /** Time difference from the last measurement in ms */
  uint64_t difference;
  /** Destructor to enable the inheritance */
  virtual ~Readings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_HPP_ */
