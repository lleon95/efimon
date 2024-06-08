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

  this->Reset();
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

  return Status{};
}

Status NVIDIAMeterObserver::Trigger() { return Status{}; }

std::vector<Readings*> NVIDIAMeterObserver::GetReadings() {
  return std::vector<Readings*>{static_cast<Readings*>(&(this->readings_))};
}
Status NVIDIAMeterObserver::SelectDevice(const uint /* device */) {
  return Status{};
}
Status NVIDIAMeterObserver::SetScope(const ObserverScope /* scope */) {
  return Status{};
}
Status NVIDIAMeterObserver::SetPID(const uint /* pid */) { return Status{}; }

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
