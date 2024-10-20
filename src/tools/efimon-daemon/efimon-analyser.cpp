/**
 * @file efimon-anlyser.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Defines the process daemon attachable class to control workers
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include "efimon-daemon/efimon-analyser.hpp"  // NOLINT

#include <algorithm>
#include <efimon/proc/cpuinfo.hpp>

#include "efimon-daemon/efimon-worker.hpp"  // NOLINT
#include "macro-handling.hpp"               // NOLINT

namespace efimon {

class SocketInfo {
 public:
  /** Default constructor */
  SocketInfo() = default;

  /** Return the number of sockets */
  int GetNumSockets() {
    std::scoped_lock slock(this->info_mutex_);
    return info_.GetNumSockets();
  }

  /** Refresh values for detecting the microarchitecture */
  Status Refresh() {
    std::scoped_lock slock(this->info_mutex_);
    return info_.Refresh();
  }

  /** Get the mean frequencies */
  std::vector<float> GetSocketMeanFrequency() {
    std::scoped_lock slock(this->info_mutex_);
    return info_.GetSocketMeanFrequency();
  }

 private:
  /** CPU Info object which should be a singleton */
  CPUInfo info_;
  /** Mutex for thread-safety */
  std::mutex info_mutex_;
};

/* SocketInfo must be a singleton */
static SocketInfo socket_info_{};

EfimonAnalyser::EfimonAnalyser() : sys_running_{false} {
  this->ipmi_meter_ = CreateIfEnabled<IPMIMeterObserver, kEnableIpmi>();
  this->rapl_meter_ = CreateIfEnabled<RAPLMeterObserver, kEnableRapl>();
  this->proc_sys_meter_ = CreateIfEnabled<ProcStatObserver, true>(
      0, efimon::ObserverScope::SYSTEM, 1);

  // Reserve space and clean up results
  this->readings_.resize(EfimonAnalyser::LAST_READINGS, nullptr);

  // Disable debug by default
  this->enable_debug_ = false;
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
                                         const uint pid, const uint delay,
                                         const uint samples,
                                         const bool enable_perf,
                                         const uint freq) {
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
  return this->proc_workers_[pid]->Start(delay, samples, enable_perf, freq);
}

Status EfimonAnalyser::CheckWorkerThread(const uint pid) {
  if (this->proc_workers_.end() == this->proc_workers_.find(pid)) {
    return Status{Status::NOT_FOUND,
                  std::to_string(static_cast<uint>(Status::NOT_FOUND))};
  }

  return this->proc_workers_[pid]->State();
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

void EfimonAnalyser::EnableDebug() { this->enable_debug_ = true; }

bool EfimonAnalyser::IsDebugged() { return this->enable_debug_; }

Status EfimonAnalyser::RefreshIPMI() {
  std::scoped_lock slock(this->sys_mutex_);
  return TriggerIfEnabled(this->ipmi_meter_);
}

Status EfimonAnalyser::RefreshRAPL() {
  std::scoped_lock slock(this->sys_mutex_);
  return TriggerIfEnabled(this->rapl_meter_);
}

Status EfimonAnalyser::RefreshProcSys() {
  std::scoped_lock slock(this->sys_mutex_);
  Status status = TriggerIfEnabled(this->proc_sys_meter_);

  socket_info_.Refresh();
  std::vector<float> socket_means = socket_info_.GetSocketMeanFrequency();

  auto cpu_readings =
      GetReadingsIfEnabled<CPUReadings, true>(this->proc_sys_meter_, 0);
  if (cpu_readings) {
    cpu_readings->socket_frequency = socket_means;
  }

  return status;
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
    EFM_DEBUG(this->enable_debug_, "System Updated");
  }
}

}  // namespace efimon
