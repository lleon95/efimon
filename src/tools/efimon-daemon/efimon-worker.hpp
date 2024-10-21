/**
 * @file efimon-worker.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Defines the process daemon attachable worker
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef SRC_TOOLS_EFIMON_DAEMON_EFIMON_WORKER_HPP_
#define SRC_TOOLS_EFIMON_DAEMON_EFIMON_WORKER_HPP_

#include <atomic>
#include <efimon/asm-classifier.hpp>
#include <efimon/logger.hpp>
#include <efimon/logger/csv.hpp>
#include <efimon/logger/macros.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings/cpu-readings.hpp>
#include <efimon/readings/instruction-readings.hpp>
#include <efimon/status.hpp>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT
#include <utility>
#include <vector>

namespace efimon {

class EfimonAnalyser;

/**
 * @brief EfiMon Worker
 *
 * This is an auxiliary class that perform process-specific measurements of the
 * CPU and other metrics.
 *
 * It is in charge of running all the process-scoped Observers and perform the
 * corresponding logging of the results, joining the process-specific
 * measurements with the system-wide ones from the EfimonAnalyser.
 *
 * It manages an internal worker thread. In the future, we plan to move this
 * to a queue-based execution to avoid overthreading.
 */
class EfimonWorker {
 public:
  /**
   * @brief Construct a new Efimon Worker
   *
   * Initialises all the worker. It includes having a dummy worker, given that
   * the PID is invalid. This is with the purpose of having dummy construction.
   *
   * Its use is discouraged.
   */
  EfimonWorker();

  /**
   * @brief Construct a new Efimon Worker object
   *
   * Move constructor implementation, which performs a move semantics except
   * for the worker thread.
   *
   * @param worker worker instance to move from.
   */
  explicit EfimonWorker(EfimonWorker &&worker);

  /**
   * @brief Construct a new Efimon Worker object
   *
   * Default constructor for the EfimonWorker. It initialises the worker class
   * for measurements.
   *
   * The construction does not start the measurements.
   *
   * @param name logfile to write onto
   * @param pid PID of the process to analyse
   * @param analyser Parent EfimonAnalyser instance
   */
  EfimonWorker(const std::string &name, const uint pid,
               EfimonAnalyser *analyser);

  // TODO(lleon): Try to simplify the API for configuring meters
  /**
   * @brief Start the worker thread
   *
   * This starts the measurements by launching a worker thread
   *
   * @param delay how often to take measurements in seconds
   * @param samples number of samples to take
   * @param enable_perf enable perf for instruction analysis
   * @param freq frequency of perf sampling (if enabled)
   * @param delay_perf time window for analysing
   * @return Status
   */
  Status Start(const uint delay, const uint samples,
               const bool enable_perf = false, const uint freq = 0,
               const uint delay_perf = 0);

  /**
   * @brief Stops the worker thread
   *
   * This stops the measurements
   *
   * @return Status
   */
  Status Stop();

  /**
   * @brief Get the state of the worker
   *
   * @return Status. It writes on the Status code Status::RUNNING and
   * Status::STOPPED. Status::STOPPED is the condition when the sampling
   * has finished.
   */
  Status State();

  /**
   * @brief Destroy the Efimon Worker object
   */
  virtual ~EfimonWorker();

 private:
  /** Filename to register the logs */
  std::string name_;

  // Running variables
  /** Process PID to analyse */
  uint pid_;
  /** Number of samples to take */
  uint samples_;
  /** Variable holding whether the worker is running or not */
  std::atomic<bool> running_;
  /** Parent EfimonAnalyser instance: borrowed */
  EfimonAnalyser *analyser_;
  /** Worker Thread */
  std::unique_ptr<std::thread> thread_;

  // Meter Instances
  /** Observer for procstat */
  std::shared_ptr<Observer> proc_meter_;
  /** Observer for perf record */
  std::shared_ptr<Observer> perf_record_meter_;
  /** Observer for perf annotate */
  std::shared_ptr<Observer> perf_annotate_meter_;

  /** Mutex for thread-safety */
  std::mutex mutex_;

  // Result instances
  /** CPU readings instance for procstat */
  CPUReadings *cpu_usage_;
  /** Instructions readings instance for perf */
  InstructionReadings *instructions_samples_;

  // Refresh functions
  /** Refresh the procstat measurements */
  Status RefreshProcStat();

  // Auxiliary logging functions
  /** Log table with all fields required by a log line */
  std::vector<Logger::MapTuple> log_table_;
  /** Create the log table structure, leading to log_table_ */
  Status CreateLogTable();
  /** Register the logs and writes the CSV file */
  Status LogReadings(CSVLogger &logger);  // NOLINT

  // Workers
  /** Worker function */
  void ProcStatsWorker(const uint delay);
};
}  // namespace efimon

#endif  // SRC_TOOLS_EFIMON_DAEMON_EFIMON_WORKER_HPP_
