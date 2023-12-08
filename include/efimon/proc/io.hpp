/**
 * @file io.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer to query the /proc/pid/io
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PROC_IO_HPP_
#define INCLUDE_EFIMON_PROC_IO_HPP_

#include <vector>

#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/io-readings.hpp>
#include <efimon/status.hpp>

namespace efimon {

/**
 * @brief Observer class that queries /proc/pid/io
 *
 * This measures the IO usage
 */
class ProcIOObserver : public Observer {
 public:
  ProcIOObserver() = delete;

  /**
   * @brief Payload structure extracted from /proc/pid/io
   *
   */
  typedef struct {
    /** The number of bytes which this task has caused to be read from storage
     */
    uint64_t rchar;
    /** The number of bytes which this task has caused, or shall cause to be
    written to disk.*/
    uint64_t wchar;
  } ProcIOData;

  /**
   * @brief Construct a new Proc IO Observer
   *
   * @param pid process id (it is specific to processes)
   * @param scope if defined as ObserverScope::SYSTEM, pid is ignored
   * @param interval interval of how often the proc is queried. 0 for manual
   * query
   */
  ProcIOObserver(const uint pid, const ObserverScope scope,
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
   * The order will be 0: I/O Readings
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure
   *
   * This method can be implemented or not, depending on the type of observer.
   * Some valid observers are the IO (measuring a socket or core)
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
   * @brief Destroy the Proc IO Observer object
   */
  virtual ~ProcIOObserver();

 private:
  /** Process aliveness */
  bool alive_;
  /** Proccess data payload when reading /proc/pid/io */
  ProcIOData proc_data_;
  /** Readings from the I/O */
  IOReadings io_readings_;
  /** Uptime */
  uint64_t uptime_;

  /**
   * @brief Get the system uptime
   *
   * @return uint64_t timestamp relative to the system uptime in milliseconds
   */
  uint64_t GetUptime();

  /**
   * @brief Read the /proc/pid/io file
   *
   */
  void GetProcIO();

  /**
   * @brief Check if the process is still alive
   */
  void CheckAlive();

  /**
   * @brief Translate the readings from ProcIOData to IOReadings
   */
  void TranslateReadings() noexcept;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PROC_IO_HPP_ */
