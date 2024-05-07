/**
 * @file fan-readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings (Fans)
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_READINGS_FAN_READINGS_HPP_
#define INCLUDE_EFIMON_READINGS_FAN_READINGS_HPP_

#include <cstdint>
#include <efimon/readings.hpp>
#include <vector>

namespace efimon {

/**
 * @brief Readings specific to FAN measurements
 */
struct FanReadings : public Readings {
  /** Average speed of all the fans in RPM  */
  float overall_speed;
  /** Fan speeds in RPM */
  std::vector<float> fan_speeds;
  virtual ~FanReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_FAN_READINGS_HPP_ */
