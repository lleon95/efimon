/**
 * @file ipmi.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Power consumption wrapper for IPMI-capable systems.
 * It gets the energy for all power supply units (PSU)
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <cstdint>
#include <efimon/power/ipmi.hpp>
#include <efimon/status.hpp>
#include <fstream>
#include <string>
#include <third-party/pstream.hpp>
#include <vector>

namespace efimon {

extern uint64_t GetUptime();

/** A maximum number of PSUs supported in a single system */
static constexpr int kMaxPSU = 100;

static constexpr char kIPMIInfoCmd[] = "ipmi-oem dell power-supply-info";
static constexpr char kIPMIPwrCmd[] =
    "ipmi-oem dell get-instantaneous-power-consumption-data";
static constexpr char kIPMISensorCmd[] = "ipmi-sensors | grep Fan";

IPMIMeterObserver::IPMIMeterObserver(const uint /* pid */,
                                     const ObserverScope scope,
                                     const uint64_t interval)
    : Observer{}, valid_{false}, psu_id_{kMaxPSU}, num_psus_{0}, max_power_{} {
  uint64_t type = static_cast<uint64_t>(ObserverType::PSU) |
                  static_cast<uint64_t>(ObserverType::POWER) |
                  static_cast<uint64_t>(ObserverType::INTERVAL);

  this->interval_ = interval;
  this->pid_ = getpid();

  if (ObserverScope::SYSTEM != scope) {
    throw Status{Status::INVALID_PARAMETER, "Process-scope is not supported"};
  }

  this->caps_.emplace_back();
  this->caps_[0].type = type;

  Status st = this->GetInfo();
  if (Status::OK != st.code) {
    throw Status{Status::ACCESS_DENIED, "Cannot get info from IPMI"};
  }

  this->GetInfo();
  this->Reset();
  this->Trigger();

  this->valid_ = false;
}

Status IPMIMeterObserver::GetInfo() {
  /* Create the command from a tempfile name */
  std::string command = std::string(kIPMIInfoCmd);

  /* Execute the command */
  redi::ipstream ip(command, redi::pstreambuf::pstdout);
  if (!ip.is_open()) {
    return Status{Status::FILE_ERROR, "Cannot execute ipmi info command"};
  }

  this->max_power_.clear();
  this->num_psus_ = 0;

  std::string payload;
  while (std::getline(ip, payload)) {
    std::string::size_type idx_word, idx_colon, idx_watts;
    idx_word = payload.find("Rated Watts");
    idx_colon = payload.find(": ");
    idx_watts = payload.find("W");

    if (std::string::npos != idx_word) {
      ++this->num_psus_;
      std::string::size_type start_offset = idx_colon + 2;
      std::string::size_type payload_size = idx_watts - 1 - start_offset;
      float val = std::stof(payload.substr(idx_colon + 2, payload_size));
      this->max_power_.push_back(val);
    }
  }

  /* Remove after use */
  if (!this->num_psus_) {
    return Status{Status::NOT_FOUND, "Cannot find compatible PSUs"};
  }

  return Status{};
}

Status IPMIMeterObserver::Trigger() {
  Status st{};
  /* Set readings common metadata */
  auto time = GetUptime();
  this->readings_.type = static_cast<uint64_t>(ObserverType::PSU) |
                         static_cast<uint64_t>(ObserverType::POWER);
  this->readings_.difference = time - this->readings_.timestamp;
  this->readings_.timestamp = time;
  this->readings_.overall_power = 0;

  /* Get fan speed */
#ifndef ENABLE_IPMI_SENSORS
  st = this->GetFanSpeed();
  if (st.code != Status::OK) {
    return st;
  }
#endif /* ENABLE_IPMI_SENSORS */

  /* Check if the parse is for a single PSU */
  if (this->psu_id_ < this->num_psus_) {
    st = this->GetPower(this->psu_id_);
    this->ParseResults(this->psu_id_);
    this->valid_ = true;
    return st;
  }

  /* Get for all PSUs */
  for (uint i = 0; i < this->num_psus_; ++i) {
    st = this->GetPower(i);
    this->ParseResults(i);
  }

  this->valid_ = true;
  return st;
}

Status IPMIMeterObserver::GetPower(const uint psu_id) {
  std::string command =
      std::string(kIPMIPwrCmd) + " " + std::to_string(psu_id + 1);

  /* Execute the command */
  redi::ipstream ip(command, redi::pstreambuf::pstdout);

  /* Parse the output */
  if (!ip.is_open()) {
    return Status{Status::NOT_FOUND, "The IPMI power file cannot be opened"};
  }

  std::string payload;
  uint occurrences = 0;
  while (std::getline(ip, payload)) {
    std::string::size_type idx_word, idx_colon, idx_watts;
    idx_word = payload.find("Instantaneous Power");
    idx_colon = payload.find(": ");
    idx_watts = payload.find("W");

    if (std::string::npos != idx_word) {
      std::string::size_type start_offset = idx_colon + 2;
      std::string::size_type payload_size = idx_watts - 1 - start_offset;
      float val = std::stof(payload.substr(idx_colon + 2, payload_size));
      this->readings_.psu_power.at(psu_id) = val;
      this->readings_.overall_power += val;
      ++occurrences;
    }
  }

  if (!occurrences) {
    return Status{Status::NOT_FOUND,
                  std::string("Cannot get the consumption of the PSU") +
                      std::to_string(psu_id)};
  }

  return Status{};
}

Status IPMIMeterObserver::GetFanSpeed() {
  Status ret{};

  /* Execute the command */
  redi::ipstream ip(kIPMISensorCmd, redi::pstreambuf::pstdout);

  /* Parse the output */
  if (!ip.is_open()) {
    return Status{Status::NOT_FOUND, "The IPMI sensor cannot be opened"};
  }

  /* Clean the vector */
  this->fan_readings_.fan_speeds.clear();
  float speed = 0.f;

  std::string payload;
  while (std::getline(ip, payload)) {
    std::string::size_type idx_bar = 0, idx_rpm;

    /* Find the RPM keyword */
    idx_rpm = payload.find("RPM");
    if (std::string::npos != idx_rpm) {
      continue;
    }

    /* Find the third bar */
    std::string substpayload = payload;
    for (int i = 0; i < 3; ++i) {
      idx_bar = substpayload.find("|");
      substpayload = substpayload.substr(idx_bar + 1);
    }
    /* Find the fourth relative to the substring */
    idx_bar = substpayload.find("|");
    substpayload = substpayload.substr(0, idx_bar - 1);

    float val = std::stof(substpayload);
    speed += val;
    this->fan_readings_.fan_speeds.emplace_back(val);
  }

  this->fan_readings_.overall_speed =
      speed / this->fan_readings_.fan_speeds.size();

  return ret;
}

void IPMIMeterObserver::ParseResults(const uint psu_id) {
  if (!this->valid_) return;
  float energy =
      this->readings_.psu_power.at(psu_id) * this->readings_.difference * 1e-3;
  this->readings_.overall_energy += energy;
  this->readings_.psu_energy.at(psu_id) += energy;
}

std::vector<Readings*> IPMIMeterObserver::GetReadings() {
  return std::vector<Readings*>{static_cast<Readings*>(&(this->readings_)),
                                static_cast<Readings*>(&(this->fan_readings_))};
}

Status IPMIMeterObserver::SelectDevice(const uint device) {
  this->psu_id_ = device;
  return Status{};
}

Status IPMIMeterObserver::SetScope(const ObserverScope scope) {
  if (ObserverScope::SYSTEM == scope) return Status{};
  return Status{Status::NOT_IMPLEMENTED, "The scope is only set to SYSTEM"};
}

Status IPMIMeterObserver::SetPID(const uint /* pid */) {
  return Status{Status::NOT_IMPLEMENTED,
                "It is not possible to set a PID in a SYSTEM wide Observer"};
}

ObserverScope IPMIMeterObserver::GetScope() const noexcept {
  return ObserverScope::SYSTEM;
}

uint IPMIMeterObserver::GetPID() const noexcept { return 0; }

const std::vector<ObserverCapabilities>& IPMIMeterObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status IPMIMeterObserver::GetStatus() { return Status{}; }

Status IPMIMeterObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status IPMIMeterObserver::ClearInterval() {
  return Status{Status::NOT_IMPLEMENTED,
                "The clear interval is not implemented yet"};
}

Status IPMIMeterObserver::Reset() {
  this->readings_.type = static_cast<uint>(ObserverType::NONE);
  this->readings_.timestamp = 0;
  this->readings_.difference = 0;
  this->readings_.overall_power = 0;
  this->readings_.overall_energy = 0;
  this->readings_.psu_power.clear();
  this->readings_.psu_energy.clear();
  this->readings_.psu_power.resize(this->num_psus_, 0.f);
  this->readings_.psu_energy.resize(this->num_psus_, 0.f);
  this->readings_.psu_max_power = this->max_power_;
  this->fan_readings_.overall_speed = 0.f;
  this->fan_readings_.fan_speeds.clear();
  return Status{};
}

IPMIMeterObserver::~IPMIMeterObserver() {}

} /* namespace efimon */
