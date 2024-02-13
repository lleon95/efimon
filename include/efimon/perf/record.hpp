/**
 * @file record.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer to query the perf record command. It works as a wrapper
 * for other functions like perf annotate
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PERF_RECORD_HPP_
#define INCLUDE_EFIMON_PERF_RECORD_HPP_

#include <filesystem>
#include <string>
#include <vector>

#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/status.hpp>

namespace efimon {

/**
 * @brief Observer class that executes and queries the perf record command
 *
 * Gets information about the process in terms of performance, like
 * instructions execute, calls to other functions, and so on. This is a
 * built-in kernel friendly profiler.
 */
class PerfRecordObserver : public Observer {
 public:
  PerfRecordObserver() = delete;

  /**
   * @brief Construct a new perf record observer
   *
   * @param pid process id to attach in
   * @param scope only ObserverScope::PROCESS is valid
   * @param interval interval of how often the proc is queried in milliseconds.
   * 0 for manual query. This argument is later truncated to seconds. The value
   * will be ceiled.
   * @param frequency sampling frecuency in Hz of the perf profiler (how often
   * it samples)
   * @param no_dispose no dispose the results
   */
  PerfRecordObserver(const uint pid, const ObserverScope scope,
                     const uint64_t interval, const uint64_t frequency,
                     const bool no_dispose = false);

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
   * The order will be 0: binary perf file (T.B.D)
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure
   *
   * Selects the CPU core affinity. This is to be implemented
   *
   * @param device device enumeration
   * @return Status of the transaction
   */
  Status SelectDevice(const uint device) override;

  /**
   * @brief Set the Scope of the Observer instance
   *
   * This is not implemented for this class and its use won't affect the
   * overall behaviour of the instance.
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
   * @return process ID. Not valid
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
   * Sets how often the observer will be refreshed. The value is ceiled to
   * the following integer second.
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
  virtual ~PerfRecordObserver();

 private:
  /** There are valid results */
  bool valid_;
  /** Process ID */
  uint pid_;
  /** Sampling frequency: how often to sample during the interval */
  uint64_t frequency_;
  /** Command to perform the perf record */
  std::string perf_cmd_;
  /** Path to the perf.data file */
  std::string path_to_perf_data_;
  /** Path to the temporary folder */
  std::filesystem::path tmp_folder_path_;
  /** No dispose results */
  bool no_dispose_;

  /**
   * @brief Fills the perf command based on the provided arguments
   */
  void MakePerfCommand();

  /**
   * @brief Moves the perf.data file to a temporary file
   *
   * @param ipath path to load the perf data
   * @param opath path to save the perf data
   */
  void MovePerfData(const std::filesystem::path& ipath,
                    const std::filesystem::path& opath);

  /**
   * @brief Create a Temporary Folder object
   */
  void CreateTemporaryFolder();

  /**
   * @brief Releases the Temporary Folder object
   */
  void DisposeTemporaryFolder();
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PERF_RECORD_HPP_ */
