/**
 * @file record-readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings about the
 * records executed on the CPU
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PERF_RECORD_READINGS_HPP_
#define INCLUDE_EFIMON_PERF_RECORD_READINGS_HPP_

#include <efimon/readings.hpp>
#include <string>

namespace efimon {

/**
 * @brief Readings specific to perf record
 */
struct RecordReadings : public Readings {
  /** Path to the perf data */
  std::string perf_data_path;
  /** Destructor for overloading */
  virtual ~RecordReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PERF_RECORD_READINGS_HPP_ */
