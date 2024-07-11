/**
 * @file nvidia.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief NVIDIA GPU analyser: consumption in RAM, usage and power (if
 * available)
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <algorithm>
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
    : Observer{}, valid_{false}, init_{false} {
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

  /* Sets the device equal to device for monitoring all.
     Otherwise, account the first */
  this->num_devices_ = this->GetGPUCount();
  this->device_ = this->num_devices_;

  auto st = this->Reset();
  if (Status::OK != st.code) throw st;
}

const std::vector<ObserverCapabilities>& NVIDIAMeterObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status NVIDIAMeterObserver::Reset() {
  nvmlReturn_t res = NVML_SUCCESS;

  if (!this->init_) {
    for (uint device = 0; device < this->num_devices_; device++) {
      res =
          nvmlDeviceGetHandleByIndex_v2(device, &this->device_handles_[device]);
      if (NVML_SUCCESS != res) {
        return Status{Status::CANNOT_OPEN, "Cannot get the device handle for " +
                                               std::to_string(device)};
      }

      res = nvmlDeviceSetAccountingMode(this->device_handles_[device],
                                        NVML_FEATURE_ENABLED);
      if (NVML_SUCCESS != res) {
        return Status{
            Status::CONFIGURATION_ERROR,
            "Cannot configure the NVML to enable the accounting mode"};
      }
    }
    this->init_ = true;
    this->readings_.gpu_usage.resize(this->num_devices_);
    this->readings_.gpu_mem_usage.resize(this->num_devices_);
    this->readings_.gpu_power.resize(this->num_devices_);
    this->readings_.gpu_energy.resize(this->num_devices_);
    this->readings_.clock_speed_sm.resize(this->num_devices_);
    this->readings_.clock_speed_mem.resize(this->num_devices_);
  }

  this->readings_.timestamp = GetUptime();
  this->readings_.difference = 0;
  std::fill(this->readings_.gpu_usage.begin(), this->readings_.gpu_usage.end(),
            0.f);
  std::fill(this->readings_.gpu_mem_usage.begin(),
            this->readings_.gpu_mem_usage.end(), 0.f);
  std::fill(this->readings_.gpu_power.begin(), this->readings_.gpu_power.end(),
            0.f);
  std::fill(this->readings_.gpu_energy.begin(),
            this->readings_.gpu_energy.end(), 0.f);
  std::fill(this->readings_.clock_speed_sm.begin(),
            this->readings_.clock_speed_sm.end(), 0.f);
  std::fill(this->readings_.clock_speed_mem.begin(),
            this->readings_.clock_speed_mem.end(), 0.f);
  this->readings_.overall_memory = 0.f;
  this->readings_.overall_usage = 0.f;
  this->readings_.overall_power = 0.f;

  return Status{};
}

Status NVIDIAMeterObserver::GetProcessStats(const uint pid, const uint device) {
  bool found = false;
  nvmlReturn_t res = NVML_SUCCESS;
  uint size = kNumProcessLimit;
  uint i = 0;

  res = nvmlDeviceGetProcessUtilization(this->device_handles_[device],
                                        this->running_processes_, &size, 0);
  if (NVML_SUCCESS != res) {
    return Status{
        Status::CANNOT_OPEN,
        "Cannot read process utilisation on device " + std::to_string(device)};
  }

  for (; i < size; ++i) {
    if (pid == this->running_processes_[i].pid) {
      found = true;
      break;
    }
  }

  if (!found) {
    this->readings_.gpu_usage[device] = 0.f;
    this->readings_.gpu_mem_usage[device] = 0.f;
    this->readings_.gpu_power[device] = 0.f;
    this->readings_.gpu_energy[device] = 0.f;
    return Status{};
  }

  float usage = static_cast<float>(this->running_processes_[i].smUtil);
  float memory = static_cast<float>(this->running_processes_[i].memUtil) / 10.f;
  this->readings_.overall_usage += usage;
  this->readings_.overall_memory += memory;
  this->readings_.gpu_usage[device] = usage;
  this->readings_.gpu_mem_usage[device] = memory;

  return Status{};
}

Status NVIDIAMeterObserver::GetSystemStats(const uint device) {
  nvmlReturn_t res = NVML_SUCCESS;
  uint clock_mhz_sm = 0, clock_mhz_mem = 0;
  Status st{};
  unsigned long long energy_usage;  // NOLINT
  res = nvmlDeviceGetUtilizationRates(this->device_handles_[device],
                                      &this->sys_usage_);
  if (NVML_SUCCESS != res) {
    return Status{Status::CANNOT_OPEN,
                  "Cannot get GPU system utilisation stats"};
  }

  this->readings_.gpu_usage[device] = static_cast<float>(this->sys_usage_.gpu);
  this->readings_.gpu_mem_usage[device] =
      static_cast<float>(this->sys_usage_.memory) / 10.f;
  this->readings_.overall_usage += this->readings_.gpu_usage[device];
  this->readings_.overall_memory += this->readings_.gpu_mem_usage[device];

  /* Get Energy in Joules */
  res = nvmlDeviceGetTotalEnergyConsumption(this->device_handles_[device],
                                            &energy_usage);
  float new_energy = static_cast<float>(energy_usage);
  float power_usage = (new_energy - this->readings_.gpu_energy[device]) /
                      static_cast<float>(this->readings_.difference);

  this->readings_.gpu_power[device] = NVML_SUCCESS != res ? 0.f : power_usage;
  this->readings_.overall_power += this->readings_.gpu_power[device];

  this->readings_.gpu_energy[device] = new_energy;
  res = nvmlDeviceGetClockInfo(this->device_handles_[device], NVML_CLOCK_SM,
                               &clock_mhz_sm);
  if (NVML_SUCCESS != res) {
    clock_mhz_sm = 0;
  }

  res = nvmlDeviceGetClockInfo(this->device_handles_[device], NVML_CLOCK_MEM,
                               &clock_mhz_mem);
  if (NVML_SUCCESS != res) {
    clock_mhz_mem = 0;
  }

  this->readings_.clock_speed_sm[device] = static_cast<float>(clock_mhz_sm);
  this->readings_.clock_speed_mem[device] = static_cast<float>(clock_mhz_mem);

  return st;
}

Status NVIDIAMeterObserver::Trigger() {
  Status st{};
  auto time = GetUptime();
  this->readings_.difference = time - this->readings_.timestamp;
  this->readings_.timestamp = time;
  this->readings_.overall_memory = 0.f;
  this->readings_.overall_usage = 0.f;
  this->readings_.overall_power = 0.f;

  if (this->device_ >= this->num_devices_) {
    for (uint device = 0; device < this->num_devices_; device++) {
      if (ObserverScope::PROCESS == this->caps_[0].scope) {
        st = this->GetProcessStats(this->pid_, device);
      } else {
        st = this->GetSystemStats(device);
      }
    }
  } else {
    if (ObserverScope::PROCESS == this->caps_[0].scope) {
      st = this->GetProcessStats(this->pid_, this->device_);
    } else {
      st = this->GetSystemStats(this->device_);
    }
  }

  return st;
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

uint32_t NVIDIAMeterObserver::GetGPUCount() {
  uint32_t res;
  nvmlDeviceGetCount_v2(&res);
  return res;
}

} /* namespace efimon */
