/**
 * @file intel.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Power consumption wrapper for Intel Processors. In includes many
 * amenities such as CPU utilisation, Bandwidth, Power Consumption (if
 * available in the motherboard, memory hiearchy details like hits and
 * misses, and so on).
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_POWER_INTEL_HPP_
#define INCLUDE_EFIMON_POWER_INTEL_HPP_

#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/cpu-readings.hpp>
#include <vector>

namespace efimon {

/**
 * @brief Observer class that wraps the Intel PCM and queries several
 * measurements such as Power Consumption in CPU, RAM, and Bandwidth.
 *
 * Gets information (if available in the system) about:
 *
 * - Power consumption: CPU, RAM
 * - DRAM and I/O bandwidth
 * - Memory Hiearchy: hits / misses
 */
class IntelMeterObserver : public Observer {
 public:
  IntelMeterObserver() = delete;

  /**
   * @brief Constructor for the Intel Meter Observer
   *
   * If there is an existing instance of IntelMeterObserver, it works as a
   * wrapper, behaving as a singleton class.
   *
   * @param pid process id to attach in (not taken at the moment)
   * @param scope only ObserverScope::SYSTEM is valid
   * @param interval interval of how often the profiler is queried in
   * milliseconds. 0 for manual query.
   */
  IntelMeterObserver(const uint pid, const ObserverScope,
                     const uint64_t interval);

  /**
   * @brief Manually triggers the update in case that there is no interval
   *
   * @return Status of the transaction
   */
  Status Trigger() override;

  /**
   * @brief Get the Readings from the Observer
   *
   * Before reading it, the interval must be finished or the
   * Observer::Trigger() method must be invoked before calling this method
   *
   * @return std::vector<Readings*> vector of readings from the observer.
   * In this case, the Readings* can be dynamic-casted to:
   *
   * - CPUReadings. The usage metrics correspond to the number of instructions
   * per nominal cycle, not to the CPU usage itself.
   *
   * More to be defined during the way
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure (not implemented)
   *
   * @param device device enumeration
   * @return Status of the transaction
   */
  Status SelectDevice(const uint device) override;

  /**
   * @brief Set the Scope of the Observer instance (not implemented)
   *
   * @param scope instance scope, if it is process-specific or system-wide
   * @return Status of the transaction
   */
  Status SetScope(const ObserverScope scope) override;

  /**
   * @brief Set the process PID (not implemented)
   *
   * @param pid process ID
   * @return Status of the transaction
   */
  Status SetPID(const uint pid) override;

  /**
   * @brief Get the Scope of the Observer instance
   *
   * @return scope of the instance
   */
  ObserverScope GetScope() const noexcept override;

  /**
   * @brief Get the process ID in case of a process-specific instance
   *
   * @return process ID
   */
  uint GetPID() const noexcept override;

  /**
   * @brief Get the Capabilities of the Observer instance
   *
   * @return vector of capabilities
   */
  const std::vector<ObserverCapabilities>& GetCapabilities()
      const noexcept override;

  /**
   * @brief Get the Status of the Observer
   *
   * @return Status of the instance
   */
  Status GetStatus() override;

  /**
   * @brief Set the Interval in milliseconds
   *
   * Sets how often the observer will be refreshed
   *
   * @param interval time in milliseconds
   * @return Status of the setting process
   */
  Status SetInterval(const uint64_t interval) override;

  /**
   * @brief Clear the interval
   *
   * Avoids the instance to be automatically refreshed
   *
   * @return Status
   */
  Status ClearInterval() override;

  /**
   * @brief Resets the instance
   *
   * The effect is quite similar to destroy and re-construct the instance
   *
   * @return Status
   */
  Status Reset() override;

  /**
   * @brief Destroy the Observer
   */
  virtual ~IntelMeterObserver();

 private:
  /** The results are valid */
  bool valid_;
  /** Readings from CPU */
  CPUReadings readings_;

  /**
   * @brief Parses the results of Intel PCM
   *
   * Important: it is not thread-safe and depends on the lock given by
   * Trigger()
   */
  void ParseResults();
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_POWER_INTEL_HPP_
