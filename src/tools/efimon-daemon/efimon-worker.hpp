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
#include <efimon/proc/process-tree.hpp>
#include <efimon/readings/cpu-readings.hpp>
#include <efimon/readings/instruction-readings.hpp>
#include <efimon/status.hpp>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT
#include <unordered_map>
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
  enum {
    /** No assembly */
    NO_ASM = 0,
    /** Get the assembly code with perf */
    ASM_WITH_PERF,
    /** Get the assembly code with ptrace-capstone */
    ASM_WITH_PTRACE
  };
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
   * @param perf perf selector for instruction analysis
   * @param freq frequency of perf sampling (if enabled)
   * @param delay_perf time window for analysing
   * @param children number of children to analyse within the current process
   * @param cthreads number of threads to analyse within the current process
   * and the children
   * @return Status
   */
  Status Start(const uint delay, const uint samples, const uint perf = NO_ASM,
               const uint freq = 0, const uint delay_perf = 0,
               const int children = 0, const int cthreads = 0);

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
  /** Process tree instance */
  std::unique_ptr<ProcessTree> tree_;
  /** Process IDs to analyse */
  std::vector<int> cpids_;
  /** Threads IDs to analyse. The key is a PPID */
  std::unordered_map<int, std::vector<int>> pid_threads_;

  // Meter Instances
  /** Observer for procstat */
  std::unordered_map<int, std::shared_ptr<Observer>> proc_meter_;
  /** Observer for perf record */
  std::unordered_map<int, std::shared_ptr<Observer>> perf_record_meter_;
  /** Observer for perf annotate */
  std::unordered_map<int, std::shared_ptr<Observer>> perf_annotate_meter_;
  /** Observer for ptrace */
  std::unordered_map<int, std::shared_ptr<Observer>> ptrace_meter_;

  /** Mutex for thread-safety */
  std::mutex mutex_;

  // Result instances
  /** CPU readings instance for procstat */
  std::unordered_map<int, CPUReadings *> cpu_usage_;
  /** Instructions readings instance for perf */
  std::unordered_map<int, InstructionReadings *> instructions_samples_;

  // Refresh functions
  /** Refresh the procstat measurements */
  Status RefreshProcStat(const int pid);

  // Auxiliary logging functions
  /** Log table with all fields required by a log line */
  std::vector<Logger::MapTuple> log_table_;
  /** Create the log table structure, leading to log_table_ */
  Status CreateLogTable();
  /** Register the logs and writes the CSV file */
  Status LogReadings(CSVLogger &logger, const int pid);  // NOLINT

  // Workers
  /** Worker function */
  void ProcStatsWorker(const uint delay, const std::vector<int> &pids);
};
}  // namespace efimon

#endif  // SRC_TOOLS_EFIMON_DAEMON_EFIMON_WORKER_HPP_
