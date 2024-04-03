/**
 * @file psu-readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings (PSU)
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_READINGS_PSU_READINGS_HPP_
#define INCLUDE_EFIMON_READINGS_PSU_READINGS_HPP_

#include <cstdint>
#include <efimon/readings.hpp>
#include <vector>

namespace efimon {

/**
 * @brief Readings specific to PSU measurements
 */
struct PSUReadings : public Readings {
  /** Average power of all the PSUs in Watts  */
  float overall_power;
  /** Energy of all the PSUs in Joules */
  float overall_energy;
  /** Power per PSU in Watts */
  std::vector<float> psu_power;
  /** Max Power per PSU in Watts */
  std::vector<float> psu_max_power;
  /** Energy per PSU during the meter lifespan in Joules */
  std::vector<float> psu_energy;
  virtual ~PSUReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_PSU_READINGS_HPP_ */
