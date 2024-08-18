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

#include "efimon-daemon/efimon-worker.hpp"  // NOLINT

namespace efimon {

class EfimonAnalyser {
 public:
  EfimonAnalyser();

  Status StartSystemThread(const uint delay);
  Status StopSystemThread();

  Status StartWorkerThread(const std::string &name, const uint pid,
                           const uint delay);
  Status StopWorkerThread(const uint pid);

  virtual ~EfimonAnalyser() = default;

 private:
  // Running flags
  std::atomic<bool> sys_running_;
  // Meter Instances
  std::shared_ptr<Observer> proc_sys_meter_;
  std::shared_ptr<Observer> ipmi_meter_;
  std::shared_ptr<Observer> rapl_meter_;

  // Result instances
  PSUReadings *psu_readings_;
  FanReadings *fan_readings_;
  CPUReadings *cpu_energy_readings_;
  CPUReadings *cpu_usage_;

  // Refresh functions
  Status RefreshProcSys();
  Status RefreshIPMI();
  Status RefreshRAPL();

  // Workers
  void SystemStatsWorker(const int delay);

  // Running
  std::mutex sys_mutex_;
  std::unique_ptr<std::thread> sys_thread_;
  std::unordered_map<uint, EfimonWorker> proc_workers_;
};

}  // namespace efimon
#endif  // SRC_TOOLS_EFIMON_DAEMON_EFIMON_ANALYSER_HPP_
