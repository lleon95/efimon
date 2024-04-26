/**
 * @file observer.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer interface used to query sensors or meters
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_OBSERVER_HPP_
#define INCLUDE_EFIMON_OBSERVER_HPP_

#include <vector>

#include <efimon/observer-enums.hpp>
#include <efimon/readings.hpp>
#include <efimon/status.hpp>

namespace efimon {

/**
 * @brief Structure that holds the characteristics of the observer
 *
 * This contains mainly the type and the scope
 */
struct ObserverCapabilities {
  /** Type of the observer*/
  uint64_t type;
  /** Scope of the observer */
  ObserverScope scope;
};

/**
 * @brief Observer class used as interface for sensors and meters
 *
 * This class is used as interface for concrete implementations that aim to
 * read sensors, meters, etc. It has a factory with the enums of the supported
 * implementations. Moreover, there is a property that summarises the
 * capabilities, i.e. CPU metering.
 */
class Observer {
 public:
  /**
   * @brief Manually triggers the measurement in case that there is no interval
   *
   * @return Status of the transaction
   */
  virtual Status Trigger() = 0;

  /**
   * @brief Get the Readings from the Observer
   *
   * Before reading it, the interval must be finished or the
   * Observer::Trigger() method must be invoked before calling this method
   *
   * @return std::vector<Readings*> vector of readings from the observer
   */
  virtual std::vector<Readings*> GetReadings() = 0;

  /**
   * @brief Select the device to measure
   *
   * This method can be implemented or not, depending on the type of observer.
   * Some valid observers are the CPU (measuring a socket or core)
   *
   * @param device device enumeration
   * @return Status of the transaction
   */
  virtual Status SelectDevice(const uint device) = 0;

  /**
   * @brief Set the Scope of the Observer instance
   *
   * @param scope instance scope, if it is process-specific or system-wide
   * @return Status of the transaction
   */
  virtual Status SetScope(const ObserverScope scope) = 0;

  /**
   * @brief Set the process PID in case that the scope is
   * ObserverScope::PROCESS
   *
   * @param pid process ID
   * @return Status of the transaction
   */
  virtual Status SetPID(const uint pid) = 0;

  /**
   * @brief Get the Scope of the Observer instance
   *
   * @return scope of the instance
   */
  virtual ObserverScope GetScope() const noexcept = 0;

  /**
   * @brief Get the process ID in case of a process-specific instance
   *
   * @return process ID
   */
  virtual uint GetPID() const noexcept = 0;

  /**
   * @brief Get the Capabilities of the Observer instance
   *
   * @return vector of capabilities
   */
  virtual const std::vector<ObserverCapabilities>& GetCapabilities() const
      noexcept = 0;

  /**
   * @brief Get the Status of the Observer
   *
   * @return Status of the instance
   */
  virtual Status GetStatus() = 0;

  /**
   * @brief Set the Interval in milliseconds
   *
   * Sets how often the observer will be refreshed
   *
   * @param interval time in milliseconds
   * @return Status of the setting process
   */
  virtual Status SetInterval(const uint64_t interval) = 0;

  /**
   * @brief Clear the interval
   *
   * Avoids the instance to be automatically refreshed
   *
   * @return Status
   */
  virtual Status ClearInterval() = 0;

  /**
   * @brief Resets the instance
   *
   * The effect is quite similar to destroy and re-construct the instance
   *
   * @return Status
   */
  virtual Status Reset() = 0;

  /**
   * @brief Destroy the Observer
   */
  virtual ~Observer() = default;

 protected:
  /** Capabilities of the observer */
  std::vector<ObserverCapabilities> caps_;
  /** Status of the instance */
  Status status_;
  /** Process ID of the instance: in case of process-specific instances */
  uint pid_;
  /** Refresh interval */
  uint64_t interval_;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_OBSERVER_HPP_ */
