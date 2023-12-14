/**
 * @file net.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer to query the /proc/net/dev
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PROC_NET_HPP_
#define INCLUDE_EFIMON_PROC_NET_HPP_

#include <string>
#include <unordered_map>
#include <vector>

#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/net-readings.hpp>
#include <efimon/status.hpp>

namespace efimon {

/**
 * @brief Observer class that queries /proc/net/dev
 *
 * This measures the Net usage
 */
class ProcNetObserver : public Observer {
 public:
  ProcNetObserver() = delete;

  /**
   * @brief Construct a new Proc Net Observer
   *
   * @param pid process id (it is specific to processes)
   * @param scope if defined as ObserverScope::SYSTEM, pid is ignored
   * @param interval interval of how often the proc is queried. 0 for manual
   * query
   */
  ProcNetObserver(const uint pid, const ObserverScope scope,
                  const uint64_t interval);

  /**
   * @brief Manually triggers the measurement in case that there is no interval
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
   * @return std::vector<Readings> vector of readings from the observer.
   * The order will be 0: Net Readings
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure
   *
   * This method can be implemented or not, depending on the type of observer.
   * Some valid observers are the net
   *
   * @param device device enumeration
   * @return Status of the transaction
   */
  Status SelectDevice(const uint device) override;

  /**
   * @brief Set the Scope of the Observer instance
   *
   * @param scope instance scope, if it is process-specific or system-wide
   * @return Status of the transaction
   */
  Status SetScope(const ObserverScope scope) override;

  /**
   * @brief Set the process PID in case that the scope is
   * ObserverScope::PROCESS
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
   * @brief Destroy the Proc MemInfo Observer object
   */
  virtual ~ProcNetObserver();

  /**
   * @brief Get the device names
   *
   * @return std::vector<std::string> vector with device names
   */
  const std::vector<std::string>& GetDeviceNames() const noexcept;

 private:
  /** Readings from the Net */
  std::vector<NetReadings> net_readings_;
  /** Local data */
  std::unordered_map<std::string, NetReadings> data_;
  /** Uptime */
  uint64_t uptime_;
  /** Last reading uptime */
  uint64_t prev_uptime_;
  /** Device selected in the order provided in /proc/net/dev */
  uint device_;
  /** Device names */
  std::vector<std::string> device_names_;

  /**
   * @brief Get the system uptime
   *
   * @return uint64_t timestamp relative to the system uptime in milliseconds
   */
  uint64_t GetUptime();

  /**
   * @brief Read the /proc/meminfo file
   *
   */
  void GetProcNetDev();
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PROC_NET_HPP_ */
