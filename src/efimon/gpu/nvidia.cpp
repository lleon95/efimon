/**
 * @file nvidia.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief NVIDIA GPU analyser: consumption in RAM, usage and power (if
 * available)
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/gpu/nvidia.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/gpu-readings.hpp>
#include <vector>

namespace efimon {

extern uint64_t GetUptime();

NVIDIAMeterObserver::NVIDIAMeterObserver(const uint pid,
                                         const ObserverScope scope,
                                         const uint64_t interval)
    : Observer{}, valid_{false}, device_{0} {
  uint64_t type = static_cast<uint64_t>(ObserverType::GPU) |
                  static_cast<uint64_t>(ObserverType::INTERVAL);
  this->pid_ = pid;
  this->interval_ = interval;
  this->status_ = Status{};
  this->caps_.emplace_back();
  this->caps_[0].type = type;
  this->caps_[0].scope = scope;

  nvmlReturn_t res = NVML_SUCCESS;
  res = nvmlInit_v2();

  if (NVML_SUCCESS != res) {
    throw Status{Status::CONFIGURATION_ERROR,
                 "Cannot initialise the NVML using nvmlInit_v2"};
  }

  auto st = this->Reset();
  if (Status::OK != st.code) throw st;
}

const std::vector<ObserverCapabilities>& NVIDIAMeterObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status NVIDIAMeterObserver::Reset() {
  nvmlReturn_t res = NVML_SUCCESS;

  res = nvmlDeviceGetHandleByIndex_v2(this->device_, &this->device_handle_);
  if (NVML_SUCCESS != res) {
    return Status{Status::CANNOT_OPEN, "Cannot get the device handle for " +
                                           std::to_string(this->device_)};
  }

  res = nvmlDeviceSetAccountingMode(this->device_handle_, NVML_FEATURE_ENABLED);
  if (NVML_SUCCESS != res) {
    return Status{Status::CONFIGURATION_ERROR,
                  "Cannot configure the NVML to enable the accounting mode"};
  }

  this->readings_.timestamp = GetUptime();
  this->readings_.difference = 0;
  this->prev_energy_ = 0.f;

  return Status{};
}

Status NVIDIAMeterObserver::GetRunningProcesses() {
  nvmlReturn_t res = NVML_SUCCESS;
  this->running_processes_ = kNumProcessLimit;

  res = nvmlDeviceGetComputeRunningProcesses(
      this->device_handle_, &this->running_processes_, this->process_info_);
  if (NVML_SUCCESS != res) {
    return Status{Status::CANNOT_OPEN, "Cannot get the running processes"};
  }

  return Status{};
}

Status NVIDIAMeterObserver::GetProcessStats(const uint pid) {
  bool found = false;
  nvmlReturn_t res = NVML_SUCCESS;

  for (uint i = 0; i < this->running_processes_; ++i) {
    if (pid == this->process_info_[i].pid) {
      found = true;
      break;
    }
  }

  if (!found) {
    return Status{Status::NOT_FOUND, "PID not found"};
  }

  res = nvmlDeviceGetAccountingStats(this->device_handle_, pid, &this->stats_);
  if (NVML_SUCCESS != res) {
    return Status{Status::CANNOT_OPEN, "Cannot get stats for the given PID"};
  }

  this->readings_.overall_usage =
      static_cast<float>(this->stats_.gpuUtilization);
  this->readings_.overall_memory =
      static_cast<float>(this->stats_.maxMemoryUsage) / 1024;

  return Status{};
}

Status NVIDIAMeterObserver::GetSystemStats() {
  nvmlReturn_t res = NVML_SUCCESS;
  uint clock_mhz_sm = 0, clock_mhz_mem = 0;
  Status st{};
  unsigned long long energy_usage;  // NOLINT
  res = nvmlDeviceGetUtilizationRates(this->device_handle_, &this->sys_usage_);
  if (NVML_SUCCESS != res) {
    return Status{Status::CANNOT_OPEN,
                  "Cannot get GPU system utilisation stats"};
  }

  this->readings_.overall_usage = static_cast<float>(this->sys_usage_.gpu);
  this->readings_.overall_memory = static_cast<float>(this->sys_usage_.memory);

  auto time = GetUptime();
  this->readings_.difference = time - this->readings_.timestamp;
  this->readings_.timestamp = time;

  /* Get Energy in Joules: use this instead of power because it is softer */
  res =
      nvmlDeviceGetTotalEnergyConsumption(this->device_handle_, &energy_usage);
  float new_energy = static_cast<float>(energy_usage);  // get micros
  float power_usage = (new_energy - this->prev_energy_) /
                      static_cast<float>(this->readings_.difference);

  this->readings_.overall_power = NVML_SUCCESS != res ? -1.f : power_usage;
  this->prev_energy_ = new_energy;

  res = nvmlDeviceGetClockInfo(this->device_handle_, NVML_CLOCK_SM,
                               &clock_mhz_sm);
  if (NVML_SUCCESS != res) {
    clock_mhz_sm = 0;
  }

  res = nvmlDeviceGetClockInfo(this->device_handle_, NVML_CLOCK_MEM,
                               &clock_mhz_mem);
  if (NVML_SUCCESS != res) {
    clock_mhz_mem = 0;
  }

  this->readings_.clock_speed_sm = static_cast<float>(clock_mhz_sm);
  this->readings_.clock_speed_mem = static_cast<float>(clock_mhz_mem);

  return st;
}

Status NVIDIAMeterObserver::Trigger() {
  if (ObserverScope::PROCESS == this->caps_[0].scope) {
    Status st;
    st = this->GetRunningProcesses();
    if (Status::OK != st.code) return st;
    return this->GetProcessStats(this->pid_);
  } else {
    return this->GetSystemStats();
  }
}

std::vector<Readings*> NVIDIAMeterObserver::GetReadings() {
  return std::vector<Readings*>{static_cast<Readings*>(&(this->readings_))};
}
Status NVIDIAMeterObserver::SelectDevice(const uint device) {
  this->device_ = device;
  return this->Reset();
}
Status NVIDIAMeterObserver::SetScope(const ObserverScope scope) {
  this->caps_[0].scope = scope;
  return Status{};
}
Status NVIDIAMeterObserver::SetPID(const uint pid) {
  this->pid_ = pid;
  return Status{};
}

ObserverScope NVIDIAMeterObserver::GetScope() const noexcept {
  return this->caps_[0].scope;
}

uint NVIDIAMeterObserver::GetPID() const noexcept { return this->pid_; }

Status NVIDIAMeterObserver::GetStatus() { return this->status_; }

Status NVIDIAMeterObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status NVIDIAMeterObserver::ClearInterval() {
  return Status{Status::NOT_IMPLEMENTED,
                "The clear interval is not implemented yet"};
}

NVIDIAMeterObserver::~NVIDIAMeterObserver() { nvmlShutdown(); }

} /* namespace efimon */
