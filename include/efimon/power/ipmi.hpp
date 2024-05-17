/**
 * @file ipmi.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Free IPMI wrapper to measure the instantaneous power of the power
 * supply units (PSU)
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_POWER_IPMI_HPP_
#define INCLUDE_EFIMON_POWER_IPMI_HPP_

#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/fan-readings.hpp>
#include <efimon/readings/psu-readings.hpp>
#include <vector>

namespace efimon {

/**
 * @brief Observer class that wraps the IPMI interface and gets the
 * energy and fan speed in a granular and general overview.
 *
 */
class IPMIMeterObserver : public Observer {
 public:
  /**
   * @brief Constructor for the IPMI Meter Observer
   *
   * @param pid process id to attach in (not taken at the moment)
   * @param scope only ObserverScope::SYSTEM is valid
   * @param interval interval of how often the profiler is queried in
   * milliseconds. 0 for manual query.
   */
  IPMIMeterObserver(const uint pid = 0,
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
   * - PSUReadings. PSU power consumption
   * - FanReadings. Fan speed readings
   *
   * More to be defined during the way
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure: in this case, the PSU to monitor
   *
   * It corresponds to the PSU id. If not invoked, it takes into account
   * all PSUs. If the number exceeds the indexation of the PSUs,
   * it takes all.
   *
   * It is not possible to select a fan.
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
  const std::vector<ObserverCapabilities>& GetCapabilities() const
      noexcept override;

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
  virtual ~IPMIMeterObserver();

 private:
  /** If true, the instance has valid measurements */
  bool valid_;
  /** selected or monitored PSU id */
  uint psu_id_;
  /** number of PSUs */
  uint num_psus_;
  /** max power per psu */
  std::vector<float> max_power_;
  /** Readings from PSU */
  PSUReadings readings_;
  /** Readings from Fan speed*/
  FanReadings fan_readings_;

  /**
   * @brief Parse the results
   *
   * @param psu_id PSU identifier
   */
  void ParseResults(const uint psu_id);

  /**
   * @brief Gets info from the IPMI about PSUs
   */
  Status GetInfo();

  /**
   * @brief Gets info from the IPMI about PSUs
   *
   * @param psu_id PSU identifier
   */
  Status GetPower(const uint psu_id);

  /**
   * @brief Gets the fan speed from IPMI
   */
  Status GetFanSpeed();
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_POWER_IPMI_HPP_
