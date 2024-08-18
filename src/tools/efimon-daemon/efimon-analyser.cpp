/**
 * @file efimon-anlyser.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Defines the process daemon attachable class to control workers
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include "efimon-daemon/efimon-analyser.hpp"  // NOLINT
#include "efimon-daemon/efimon-worker.hpp"    // NOLINT
#include "macro-handling.hpp"                 // NOLINT

namespace efimon {

EfimonAnalyser::EfimonAnalyser() : sys_running_{false} {
  this->ipmi_meter_ = CreateIfEnabled<IPMIMeterObserver, kEnableIpmi>();
  this->rapl_meter_ = CreateIfEnabled<RAPLMeterObserver, kEnableRapl>();
  this->proc_sys_meter_ = CreateIfEnabled<ProcStatObserver, true>(
      0, efimon::ObserverScope::SYSTEM, 1);

  // Reserve space and clean up results
  this->readings_.resize(EfimonAnalyser::LAST_READINGS, nullptr);
}

Status EfimonAnalyser::StartSystemThread(const uint delay) {
  if (nullptr != this->sys_thread_) {
    return Status{Status::RESOURCE_BUSY, "The thread has already started"};
  }

  EFM_INFO("Starting System Monitor");

  this->sys_running_.store(true);
  this->sys_thread_ = std::make_unique<std::thread>(
      &EfimonAnalyser::SystemStatsWorker, this, delay);
  return Status{};
}

Status EfimonAnalyser::StartWorkerThread(const std::string &name,
                                         const uint pid, const uint delay) {
  if (this->proc_workers_.end() != this->proc_workers_.find(pid)) {
    return Status{Status::RESOURCE_BUSY,
                  "The monitor has already started for the given PID: " +
                      std::to_string(pid)};
  }

  EFM_INFO("Creating Process Monitor for PID: " + std::to_string(pid));
  auto pair = this->proc_workers_.emplace(
      pid, std::make_shared<EfimonWorker>(name, pid, this));

  if (!pair.second) {
    return Status{
        Status::CANNOT_OPEN,
        "Cannot create the monitor for the given PID: " + std::to_string(pid)};
  }

  EFM_INFO("Starting Process Monitor for PID: " + std::to_string(pid));
  return this->proc_workers_[pid]->Start(delay);
}

Status EfimonAnalyser::StopSystemThread() {
  if (nullptr == this->sys_thread_) {
    return Status{Status::NOT_FOUND, "The thread was not running"};
  }

  EFM_INFO("Stopping System Monitor");

  this->sys_running_.store(false);
  this->sys_thread_->join();
  this->sys_thread_.reset();
  return Status{};
}

Status EfimonAnalyser::StopWorkerThread(const uint pid) {
  auto it = this->proc_workers_.find(pid);
  if (this->proc_workers_.end() == it) {
    return Status{Status::NOT_FOUND,
                  "No monitor linked to the given PID: " + std::to_string(pid)};
  }

  EFM_INFO("Stopping Worker Monitor for PID: " + std::to_string(pid));

  it->second->Stop();
  this->proc_workers_.erase(it);
  return Status{};
}

Status EfimonAnalyser::RefreshIPMI() {
#ifdef ENABLE_IPMI
  std::scoped_lock slock(this->sys_mutex_);
#endif
  return Status{};
}

Status EfimonAnalyser::RefreshRAPL() {
#ifdef ENABLE_RAPL
  std::scoped_lock slock(this->sys_mutex_);
  return Status{};
#endif
  return Status{};
}

Status EfimonAnalyser::RefreshProcSys() {
  std::scoped_lock slock(this->sys_mutex_);
  return TriggerIfEnabled(this->proc_sys_meter_);
  return Status{};
}

void EfimonAnalyser::SystemStatsWorker(const int delay) {
  sys_running_.store(true);

  this->sys_mutex_.lock();
  this->readings_[PSU_ENERGY_READINGS] =
      GetReadingsIfEnabled<PSUReadings, kEnableIpmi>(this->ipmi_meter_, 0);
  this->readings_[FAN_READINGS] =
      GetReadingsIfEnabled<FanReadings, kEnableIpmi>(this->ipmi_meter_, 1);
  this->readings_[CPU_ENERGY_READINGS] =
      GetReadingsIfEnabled<CPUReadings, kEnableRapl>(this->rapl_meter_, 0);
  this->readings_[CPU_USAGE_READINGS] =
      GetReadingsIfEnabled<CPUReadings, true>(this->proc_sys_meter_, 0);
  this->sys_mutex_.unlock();

  while (sys_running_.load()) {
    EFM_CHECK(RefreshProcSys(), EFM_WARN);
    EFM_CHECK(RefreshIPMI(), EFM_WARN);
    EFM_CHECK(RefreshRAPL(), EFM_WARN);

    /* Wait for the next sample */
    std::this_thread::sleep_for(std::chrono::seconds(delay));
    EFM_INFO("System Updated");
  }
}

}  // namespace efimon
