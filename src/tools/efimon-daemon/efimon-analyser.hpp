/**
 * @file efimon-analyser.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Defines the process daemon attachable class to control workers
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef SRC_TOOLS_EFIMON_DAEMON_EFIMON_ANALYSER_HPP_
#define SRC_TOOLS_EFIMON_DAEMON_EFIMON_ANALYSER_HPP_

#include <atomic>
#include <efimon/logger/macros.hpp>
#include <efimon/power/ipmi.hpp>
#include <efimon/power/rapl.hpp>
#include <efimon/proc/stat.hpp>
#include <efimon/status.hpp>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT
#include <unordered_map>
#include <vector>

namespace efimon {

class EfimonWorker;

/**
 * @brief EfiMon Analyser
 *
 * This is an auxiliary class that perform system-wide measurements of the PSU,
 * CPU, and others.
 *
 * It is also in charge of managing all the processes, receiving new monitoring
 * requests, creating workers and managing to get the measurements and stopping
 * those already deployed.
 */
class EfimonAnalyser {
 public:
  enum {
    /** PSU energy readings selector */
    PSU_ENERGY_READINGS = 0,
    /** Fan speed readings selector */
    FAN_READINGS,
    /** CPU energy readings selector */
    CPU_ENERGY_READINGS,
    /** CPU usage readings selector */
    CPU_USAGE_READINGS,
    /** Last element (which is actually a last selector) */
    LAST_READINGS
  };

  /**
   * @brief Default constructor
   *
   * Constructs a new instance, defining a non-running instance
   */
  EfimonAnalyser();

  /**
   * @brief Starts the system thread
   *
   * It starts the system measurements (a.k.a. global measurements), which
   * include total CPU usage, RAM, PSU delivery and others. These measurements
   * complement the process-specific for comparison in terms of proportions.
   *
   * @param delay how often to cycle the measurement in seconds
   * @return Status
   */
  Status StartSystemThread(const uint delay);

  /**
   * @brief Stops the system thread
   *
   * See StartSystemThread for reference.
   *
   * @return Status
   */
  Status StopSystemThread();

  /**
   * @brief Starts a worker thread
   *
   * Similar to StartSystemThread, this method starts a worker thread, creating
   * a new instance of EfimonWorker. The EfimonWorker is registered into a map
   * to link with a PID. This worker is in charge of reading the statistics for
   * the process of a given PID, gathering CPU consumption and other process-
   * specific metrics.
   *
   * @param name name of the file to log the information of the process
   * @param pid PID of the process
   * @param delay how often to trigger the measurement in seconds
   * @param samples how many samples to take
   * @param enable_perf enable or disable perf. This enables the analysis
   * of the instructions executed by the process under analysis
   * @param freq frequency of perf (if enabled)
   * @param delay_perf time window of the instructions performed by perf
   * @return Status
   */
  Status StartWorkerThread(const std::string &name, const uint pid,
                           const uint delay, const uint samples,
                           const bool enable_perf = false, const uint freq = 0,
                           const uint delay_perf = 1);

  /**
   * @brief Stops the Worker thread
   *
   * This stops the analysis of a process with a given PID and destroys the
   * worker
   *
   * @param pid PID of the process
   * @return Status
   */
  Status StopWorkerThread(const uint pid);

  /**
   * @brief Check the Worker Thread Status
   *
   * This checks the status of the worker thread. It encapsulates the
   * result in the returned Status code.
   *
   * @param pid PID of the process
   * @return Status Status::RUNNING if running, Status::STOPPED if finished
   */
  Status CheckWorkerThread(const uint pid);

  /**
   * @brief Get the Readings for the system-wide metrics
   *
   * Grabs the readings for the system-wide metrics according to the enum
   * included of this class
   * @tparam T class of metric. It automatically casts Readings to the specific
   * implementation of Readings
   * @param index metrics ID. This includes the built-in enum in this class
   * @param out output object with the results
   * @return Status
   */
  template <class T>
  Status GetReadings(const int index, T &out);  // NOLINT

  /**
   * @brief Enables the debug messages
   */
  void EnableDebug();

  /**
   * @brief Returns if the debug messages are enabled
   */
  bool IsDebugged();

  /**
   * @brief Destroy the Efimon Analyser
   */
  virtual ~EfimonAnalyser() = default;

 private:
  // Running flags
  /** Status variable indicating if the system metrics are running or not */
  std::atomic<bool> sys_running_;
  // Meter Instances
  /** Procstat metrics observer instance */
  std::shared_ptr<Observer> proc_sys_meter_;
  /** IPMI observer instance */
  std::shared_ptr<Observer> ipmi_meter_;
  /** RAPL observer instance */
  std::shared_ptr<Observer> rapl_meter_;

  // Result instances
  /** System-wide readings */
  std::vector<Readings *> readings_;

  // Refresh functions
  /** Perform the triggering of the procstat observer*/
  Status RefreshProcSys();
  /** Perform the triggering of the IPMI observer*/
  Status RefreshIPMI();
  /** Perform the triggering of the RAPL observer*/
  Status RefreshRAPL();

  // Workers
  /** Worker function */
  void SystemStatsWorker(const int delay);

  // Running
  /** Mutex to access to the system-wide metrics and instances */
  std::mutex sys_mutex_;
  /** Thread of the system-wide meters */
  std::unique_ptr<std::thread> sys_thread_;
  /** Map that links the workers with the PID */
  std::unordered_map<uint, std::shared_ptr<EfimonWorker>> proc_workers_;

  // Options
  /** Enable debug */
  bool enable_debug_;
};

template <class T>
Status EfimonAnalyser::GetReadings(const int index, T &out) {  // NOLINT
  std::scoped_lock slock(this->sys_mutex_);
  if (index >= EfimonAnalyser::LAST_READINGS || index < 0) {
    return Status{Status::INVALID_PARAMETER, "The index is out of bound"};
  }

  T *val = dynamic_cast<T *>(this->readings_[index]);
  if (val) {
    out = *val;
  } else {
    return Status{Status::NOT_FOUND, "Cannot cast the result"};
  }

  return Status{};
}

}  // namespace efimon
#endif  // SRC_TOOLS_EFIMON_DAEMON_EFIMON_ANALYSER_HPP_
