/**
 * @file instruction-readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings about the
 * instructions executed on the CPU
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_READINGS_INSTRUCTION_READINGS_HPP_
#define INCLUDE_EFIMON_READINGS_INSTRUCTION_READINGS_HPP_

#include <cstdint>
#include <efimon/asm-classifier.hpp>
#include <efimon/readings.hpp>
#include <string>
#include <tuple>
#include <unordered_map>

namespace efimon {

/**
 * @brief Readings specific to instruction analysis
 */
struct InstructionReadings : public Readings {
  /** Contains the histogram of the classification according to instruction
      type, family, data origin and probability */
  std::unordered_map<assembly::InstructionType,
                     std::unordered_map<assembly::InstructionFamily,
                                        std::unordered_map<uint8_t, float>>>
      classification;
  /** Contains the same information as above but containerised by
      instruction */
  std::unordered_map<std::string, float> histogram;
  /** Destructor to enable the inheritance */
  virtual ~InstructionReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_INSTRUCTION_READINGS_HPP_ */
