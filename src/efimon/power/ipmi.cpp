/**
 * @file ipmi.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Power consumption wrapper for IPMI-capable systems.
 * It gets the energy for all power supply units (PSU)
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <efimon/power/ipmi.hpp>
#include <efimon/status.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace efimon {

extern uint64_t GetUptime();

/** A maximum number of PSUs supported in a single system */
static constexpr int kMaxPSU = 100;

static constexpr char kIPMIInfoCmd[] = "ipmi-oem dell power-supply-info";
static constexpr char kIPMIPwrCmd[] =
    "ipmi-oem dell get-instantaneous-power-consumption-data";

IPMIMeterObserver::IPMIMeterObserver(const uint /* pid */,
                                     const ObserverScope scope,
                                     const uint64_t interval)
    : Observer{}, psu_id_{kMaxPSU}, num_psus_{0}, max_power_{} {
  uint64_t type = static_cast<uint64_t>(ObserverType::PSU) |
                  static_cast<uint64_t>(ObserverType::POWER) |
                  static_cast<uint64_t>(ObserverType::INTERVAL);

  this->interval_ = interval;

  if (ObserverScope::SYSTEM != scope) {
    throw Status{Status::INVALID_PARAMETER, "Process-scope is not supported"};
  }

  this->caps_.emplace_back();
  this->caps_[0].type = type;

  Status st = this->GetInfo();
  if (Status::OK != st.code) {
    throw Status{Status::ACCESS_DENIED, "Cannot get info from IPMI"};
  }
  this->Reset();
  this->Trigger();
  this->pid_ = getpid();
}

Status IPMIMeterObserver::GetInfo() {
  /* Create the command from a tempfile name */
  auto tmp_filename_path =
      std::filesystem::temp_directory_path() /
      (std::string("efimon-ipmi-info-") + std::to_string(this->pid_));
  std::string command =
      std::string(kIPMIInfoCmd) + " > " + std::string(tmp_filename_path);

  /* Execute the command */
  std::system(command.c_str());

  /* Parse the file */
  std::ifstream info_file;
  info_file.open(tmp_filename_path);
  if (!info_file.is_open()) {
    std::filesystem::remove(tmp_filename_path);
    return Status{Status::NOT_FOUND, "The IPMI file cannot be opened"};
  }

  this->max_power_.clear();
  this->num_psus_ = 0;

  std::string payload;
  while (std::getline(info_file, payload)) {
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
  std::filesystem::remove(tmp_filename_path);
  if (!this->num_psus_) {
    return Status{Status::NOT_FOUND, "Cannot find compatible PSUs"};
  }

  return Status{};
}

Status IPMIMeterObserver::Trigger() {
  /* Set readings common metadata */
  auto time = GetUptime();
  this->readings_.type = static_cast<uint64_t>(ObserverType::PSU) |
                         static_cast<uint64_t>(ObserverType::POWER);
  this->readings_.difference = time - this->readings_.timestamp;
  this->readings_.timestamp = time;
  this->readings_.overall_power = 0;

  /* Check if the parse is for a single PSU */
  if (this->psu_id_ < this->num_psus_) {
    this->GetPower(this->psu_id_);
    this->ParseResults(this->psu_id_);
    return Status{};
  }

  /* Get for all PSUs */
  for (uint i = 0; i < this->num_psus_; ++i) {
    this->GetPower(i);
    this->ParseResults(i);
  }

  return Status{};
}

Status IPMIMeterObserver::GetPower(const uint psu_id) {
  /* Create the command from a tempfile name */
  auto tmp_filename_path =
      std::filesystem::temp_directory_path() /
      (std::string("efimon-ipmi-power-") + std::to_string(this->pid_) +
       std::string("-p") + std::to_string(psu_id));
  std::string command = std::string(kIPMIPwrCmd) + " " +
                        std::to_string(psu_id) + " > " +
                        std::string(tmp_filename_path);

  /* Execute the command */
  std::system(command.c_str());

  /* Parse the file */
  std::ifstream info_file;
  info_file.open(tmp_filename_path);
  if (!info_file.is_open()) {
    std::filesystem::remove(tmp_filename_path);
    return Status{Status::NOT_FOUND, "The IPMI power file cannot be opened"};
  }

  std::string payload;
  uint occurrences = 0;
  while (std::getline(info_file, payload)) {
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

  /* Remove after use */
  std::filesystem::remove(tmp_filename_path);

  if (!occurrences) {
    return Status{Status::NOT_FOUND,
                  std::string("Cannot get the consumption of the PSU") +
                      std::to_string(psu_id)};
  }

  return Status{};
}

void IPMIMeterObserver::ParseResults(const uint psu_id) {
  float energy =
      this->readings_.psu_power.at(psu_id) * this->readings_.difference;
  this->readings_.overall_energy += energy;
  this->readings_.psu_energy.at(psu_id) += energy;
}

std::vector<Readings*> IPMIMeterObserver::GetReadings() {
  return std::vector<Readings*>{static_cast<Readings*>(&(this->readings_))};
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
  return Status{};
}

IPMIMeterObserver::~IPMIMeterObserver() {}

} /* namespace efimon */
