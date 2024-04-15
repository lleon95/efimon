/**
 * @file rapl.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Power consumption wrapper for RAPL-capable processors.
 * It gets the energy for all sockets if possible and depends
 * on the /proc/cpuinfo
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_POWER_RAPL_HPP_
#define INCLUDE_EFIMON_POWER_RAPL_HPP_

#include <efimon/observer.hpp>
#include <efimon/proc/cpuinfo.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/cpu-readings.hpp>
#include <vector>

namespace efimon {

/**
 * @brief Observer class that wraps the RAPL interface and gets the
 * energy in a granular and general overview.
 *
 */
class RAPLMeterObserver : public Observer {
 public:
  /**
   * @brief Constructor for the RAPL Meter Observer
   *
   * @param pid process id to attach in (not taken at the moment)
   * @param scope only ObserverScope::SYSTEM is valid
   * @param interval interval of how often the profiler is queried in
   * milliseconds. 0 for manual query.
   */
  RAPLMeterObserver(const uint pid = 0,
                    const ObserverScope = ObserverScope::SYSTEM,
                    const uint64_t interval = 0);

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
   * - CPUReadings. The metrics here corresponds to the power measurements
   * from RAPL. Usage metrics are not present. Please, use the power and
   * energy metrics only
   *
   * More to be defined during the way
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure.
   *
   * It corresponds to the socket id. If not invoked, it takes into account
   * all sockets. If the number exceeds the indexation of the sockets,
   * it takes all
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
  virtual ~RAPLMeterObserver();

 private:
  /** Instance for querying the CPUInfo */
  CPUInfo info_;
  /** The results are valid */
  bool valid_;
  /** Socket device */
  uint device_;
  /** Socket energy meters: Before */
  std::vector<double> before_socket_meters_;
  /** Socket energy meters: Current */
  std::vector<double> after_socket_meters_;
  /** Socket energy meters: Maximum Range */
  std::vector<double> max_socket_meters_;
  /** Readings from CPU */
  CPUReadings readings_;

  /**
   * @brief Get the Socket Consumption
   *
   * @param socket_id socket identifier
   */
  Status GetSocketConsumption(const uint socket_id);

  /**
   * @brief Parse the results
   *
   * @param socket_id socket identifier
   */
  void ParseResults(const uint socket_id);
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_POWER_RAPL_HPP_
