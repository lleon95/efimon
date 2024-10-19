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

class EfimonAnalyser {
 public:
  enum {
    PSU_ENERGY_READINGS = 0,
    FAN_READINGS,
    CPU_ENERGY_READINGS,
    CPU_USAGE_READINGS,
    LAST_READINGS
  };

  EfimonAnalyser();

  Status StartSystemThread(const uint delay);
  Status StopSystemThread();

  Status StartWorkerThread(const std::string &name, const uint pid,
                           const uint delay, const uint samples,
                           const bool enable_perf = false, const uint freq = 0);
  Status StopWorkerThread(const uint pid);
  Status CheckWorkerThread(const uint pid);

  template <class T>
  Status GetReadings(const int index, T &out);  // NOLINT

  void EnableDebug();
  bool IsDebugged();

  virtual ~EfimonAnalyser() = default;

 private:
  // Running flags
  std::atomic<bool> sys_running_;
  // Meter Instances
  std::shared_ptr<Observer> proc_sys_meter_;
  std::shared_ptr<Observer> ipmi_meter_;
  std::shared_ptr<Observer> rapl_meter_;

  // Result instances
  std::vector<Readings *> readings_;

  // Refresh functions
  Status RefreshProcSys();
  Status RefreshIPMI();
  Status RefreshRAPL();

  // Workers
  void SystemStatsWorker(const int delay);

  // Running
  std::mutex sys_mutex_;
  std::unique_ptr<std::thread> sys_thread_;
  std::unordered_map<uint, std::shared_ptr<EfimonWorker>> proc_workers_;

  // Options
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
