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

class EfimonWorker {
 public:
  EfimonWorker();
  explicit EfimonWorker(EfimonWorker &&worker);
  EfimonWorker(const std::string &name, const uint pid,
               EfimonAnalyser *analyser);

  Status Start(const uint delay, const bool enable_perf = false,
               const uint freq = 0);
  Status Stop();

  virtual ~EfimonWorker();

 private:
  std::string name_;

  // Running variables
  uint pid_;
  std::atomic<bool> running_;
  EfimonAnalyser *analyser_;
  std::unique_ptr<std::thread> thread_;

  // Meter Instances
  std::shared_ptr<Observer> proc_meter_;
  std::shared_ptr<Observer> perf_record_meter_;
  std::shared_ptr<Observer> perf_annotate_meter_;

  std::mutex mutex_;

  // Result instances
  CPUReadings *cpu_usage_;
  InstructionReadings *instructions_samples_;

  // Refresh functions
  Status RefreshProcStat();

  // Auxiliary logging functions
  std::vector<Logger::MapTuple> log_table_;
  Status CreateLogTable();
  Status LogReadings(CSVLogger &logger);  // NOLINT

  // Workers
  void ProcStatsWorker(const uint delay);
};
}  // namespace efimon

#endif  // SRC_TOOLS_EFIMON_DAEMON_EFIMON_WORKER_HPP_
