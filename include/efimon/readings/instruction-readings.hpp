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
#include <string>
#include <unordered_map>

#include <efimon/readings.hpp>

namespace efimon {

/**
 * @brief Readings specific to instruction analysis
 */
struct InstructionReadings : public Readings {
  /** Proportion of scalar arithmetic instructions like sub, add, mult, div.
      It includes floating-point scalar instructions */
  float scalar_arithmetic;
  /** Proportion of scalar logic instructions like and, or, not. It includes
      tests and comparisons  */
  float scalar_logic;
  /** Proportion of scalar memory loads, including direct and not direct
      memory addressing */
  float scalar_memory_loads;
  /** Other scalar instructions not found above */
  float scalar_other;
  /** Proportion of conditional branches */
  float conditional_branches;
  /** Proportion of not conditional branches */
  float unconditional_branches;
  /** Same as scalar_arithmetic but for vector instructions */
  float vector_arithmetic;
  /** Same as scalar_logic but for vector instructions */
  float vector_logic;
  /** Same as scalar_memory_loads but for vector instructions */
  float vector_memory_loads;
  /** Other scalar instructions not found above */
  float vector_other;
  /** Unclassified instructions */
  float unclassified;
  /** Contains the same information as above but containerised by
      instruction */
  std::unordered_map<std::string, float> histogram;
  /** Destructor to enable the inheritance */
  virtual ~InstructionReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_INSTRUCTION_READINGS_HPP_ */
