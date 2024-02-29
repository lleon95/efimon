/**
 * @file observer-enums.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Enum representing the observer enums
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_OBSERVER_ENUMS_HPP_
#define INCLUDE_EFIMON_OBSERVER_ENUMS_HPP_

#include <cstdint>

namespace efimon {

/**
 * @brief Enum to represent the types of observer
 *
 * An observer can have all the observer types or some of them.
 * The suggestion is to or the values
 */
enum class ObserverType {
  /** Representing a null observer: dummy observer or some of non-metering
      instance */
  NONE = 0,
  /** CPU meter */
  CPU = 1 << 0,
  /** System memory meter */
  RAM = 1 << 1,
  /** I/O meter */
  IO = 1 << 2,
  /** Network meter */
  NETWORK = 1 << 3,
  /** Video RAM meter */
  VRAM = 1 << 4,
  /** GPU processor meter */
  GPU = 1 << 5,
  /** Power meter: it does not specify what kind of source */
  POWER = 1 << 6,
  /** Interval meter: it specifies a query instance of non-metering sensor,
      like the process analyser */
  INTERVAL = 1 << 7,
  /** CPU instructions analysis */
  CPU_INSTRUCTIONS = 1 << 8,
  /** All: singleton observer */
  ALL = 1 << 31
};

/**
 * @brief Defines the scope of the observer
 *
 * The scopes are mutually exclusive and should be used just one per observer
 */
enum class ObserverScope {
  /** Observer that only measures the metrics for a single process */
  PROCESS = 1,
  /** Observer that measures the metrics for the whole system */
  SYSTEM = 2
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_OBSERVER_ENUMS_HPP_ */
