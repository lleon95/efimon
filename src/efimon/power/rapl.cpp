/**
 * @file rapl.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Power consumption wrapper for RAPL-capable processors.
 * It gets the energy for all sockets if possible and depends
 * on the /proc/cpuinfo
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <cstdint>
#include <efimon/power/rapl.hpp>
#include <efimon/status.hpp>
#include <fstream>
#include <string>
#include <vector>

namespace efimon {

extern uint64_t GetUptime();

RAPLMeterObserver::RAPLMeterObserver(const uint /* pid */,
                                     const ObserverScope scope,
                                     const uint64_t interval)
    : Observer{}, info_{}, valid_{false} {
  uint64_t type = static_cast<uint64_t>(ObserverType::CPU) |
                  static_cast<uint64_t>(ObserverType::POWER) |
                  static_cast<uint64_t>(ObserverType::INTERVAL);

  this->interval_ = interval;
  this->device_ = info_.GetNumSockets();

  if (ObserverScope::SYSTEM != scope) {
    throw Status{Status::INVALID_PARAMETER, "Process-scope is not supported"};
  }

  this->caps_.emplace_back();
  this->caps_[0].type = type;

  this->Reset();
  this->Trigger();
}

Status RAPLMeterObserver::GetSocketConsumption(const uint socket_id) {
  /* Make the powercap file */
  std::string energy_file_name = "/sys/class/powercap/intel-rapl:";
  energy_file_name += std::to_string(socket_id);
  energy_file_name += "/energy_uj";

  /* Open file */
  std::ifstream energy_file;
  energy_file.open(energy_file_name);
  if (!energy_file.is_open()) {
    return Status{Status::NOT_FOUND, "The RAPL Interface cannot be opened"};
  }

  std::string payload_uj;
  std::getline(energy_file, payload_uj);

  if (!valid_) {
    /* Read two times */
    before_socket_meters_.at(socket_id) = std::stof(payload_uj) * 1e-06;
    after_socket_meters_.at(socket_id) = std::stof(payload_uj) * 1e-06;
  } else {
    std::swap(this->before_socket_meters_, this->after_socket_meters_);
    after_socket_meters_.at(socket_id) = std::stof(payload_uj) * 1e-06;
  }

  return Status{};
}

Status RAPLMeterObserver::Trigger() {
  /* Set readings common metadata */
  auto time = GetUptime();
  this->readings_.type = static_cast<uint64_t>(ObserverType::CPU) |
                         static_cast<uint64_t>(ObserverType::POWER);
  this->readings_.difference = time - this->readings_.timestamp;
  this->readings_.timestamp = time;
  this->readings_.overall_power = 0;

  /* Check if the parse is for a single socket */
  if (this->device_ < static_cast<uint>(info_.GetNumSockets())) {
    this->GetSocketConsumption(this->device_);
    this->ParseResults(this->device_);
    this->valid_ = true;
    return Status{};
  }

  /* Get for all sockets */
  for (int i = 0; i < info_.GetNumSockets(); ++i) {
    this->GetSocketConsumption(i);
    this->ParseResults(i);
  }

  /* Get for all the sockets */
  this->valid_ = true;
  return Status{};
}

void RAPLMeterObserver::ParseResults(const uint socket_id) {
  float val = this->after_socket_meters_.at(socket_id) -
              this->before_socket_meters_.at(socket_id);
  this->readings_.socket_power.at(socket_id) = val;
  this->readings_.overall_power += val;
}

std::vector<Readings*> RAPLMeterObserver::GetReadings() {
  return std::vector<Readings*>{static_cast<Readings*>(&(this->readings_))};
}

Status RAPLMeterObserver::SelectDevice(const uint device) {
  this->device_ = device;
  return Status{};
}

Status RAPLMeterObserver::SetScope(const ObserverScope scope) {
  if (ObserverScope::SYSTEM == scope) return Status{};
  return Status{Status::NOT_IMPLEMENTED, "The scope is only set to SYSTEM"};
}

Status RAPLMeterObserver::SetPID(const uint /* pid */) {
  return Status{Status::NOT_IMPLEMENTED,
                "It is not possible to set a PID in a SYSTEM wide Observer"};
}

ObserverScope RAPLMeterObserver::GetScope() const noexcept {
  return ObserverScope::SYSTEM;
}

uint RAPLMeterObserver::GetPID() const noexcept { return 0; }

const std::vector<ObserverCapabilities>& RAPLMeterObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status RAPLMeterObserver::GetStatus() { return Status{}; }

Status RAPLMeterObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status RAPLMeterObserver::ClearInterval() {
  return Status{Status::NOT_IMPLEMENTED,
                "The clear interval is not implemented yet"};
}

Status RAPLMeterObserver::Reset() {
  this->readings_.type = static_cast<uint>(ObserverType::NONE);
  this->readings_.timestamp = 0;
  this->readings_.difference = 0;
  this->readings_.overall_usage = -1;
  this->readings_.socket_power.clear();
  this->readings_.core_usage.clear();
  this->readings_.socket_usage.clear();
  this->readings_.core_power.clear();

  this->readings_.overall_power = 0;
  this->readings_.socket_power.resize(info_.GetNumSockets(), 0.f);
  this->before_socket_meters_.resize(info_.GetNumSockets(), 0.f);
  this->after_socket_meters_.resize(info_.GetNumSockets(), 0.f);
  this->readings_.core_power.resize(info_.GetLogicalCores(), 0.f);
  return Status{};
}

RAPLMeterObserver::~RAPLMeterObserver() {}

} /* namespace efimon */
