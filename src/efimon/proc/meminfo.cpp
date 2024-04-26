/**
 * @file stat.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the /proc/pid/stat looking for metrics about RAM and CPU
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <efimon/proc/meminfo.hpp>
#include <efimon/status.hpp>

#include <unistd.h>

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define MAX_LEN_FILE_PATH 255

namespace efimon {
ProcMemInfoObserver::ProcMemInfoObserver(const uint /*pid*/,
                                         const ObserverScope scope,
                                         const uint64_t interval)
    : Observer{} {
  uint64_t type = static_cast<uint64_t>(ObserverType::CPU) |
                  static_cast<uint64_t>(ObserverType::INTERVAL);
  this->pid_ = -1;
  this->interval_ = interval;
  this->status_ = Status{};
  this->caps_.emplace_back();
  this->caps_[0].type = type;
  if (ObserverScope::SYSTEM != scope) {
    throw Status{Status::INVALID_PARAMETER, "Process-scope is not supported"};
  }
  this->caps_[0].scope = scope;
  this->Reset();
}

ProcMemInfoObserver::~ProcMemInfoObserver() {}

Status ProcMemInfoObserver::Trigger() {
  /* Update the uptime */
  GetUptime();

  /* Get the ProcMemInfo and process the results */
  GetProcMemInfo();
  TranslateReadings();

  return Status{};
}

std::vector<Readings *> ProcMemInfoObserver::GetReadings() {
  std::vector<Readings *> readings{};
  readings.push_back(&this->ram_readings_);
  return readings;
}

Status ProcMemInfoObserver::SelectDevice(const uint /*device*/) {
  return Status{Status::NOT_IMPLEMENTED,
                "Cannot select a device since it is not implemented"};
}

Status ProcMemInfoObserver::SetScope(const ObserverScope /*scope*/) {
  return Status{Status::NOT_IMPLEMENTED,
                "Cannot select a device since it is not implemented"};
}

Status ProcMemInfoObserver::SetPID(const uint /*pid*/) {
  return Status{Status::NOT_IMPLEMENTED,
                "Cannot set a PID since it is not implemented"};
}

ObserverScope ProcMemInfoObserver::GetScope() const noexcept {
  return this->caps_[0].scope;
}

uint ProcMemInfoObserver::GetPID() const noexcept { return this->pid_; }

const std::vector<ObserverCapabilities> &ProcMemInfoObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status ProcMemInfoObserver::GetStatus() { return this->status_; }

Status ProcMemInfoObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status ProcMemInfoObserver::ClearInterval() { return Status{}; }

Status ProcMemInfoObserver::Reset() {
  /* Resetting the structures is more than enough */
  std::memset(&this->proc_data_, 0, sizeof(ProcMemInfoData));
  this->ram_readings_.overall_usage = 0;
  this->ram_readings_.overall_bw = 0;
  this->ram_readings_.overall_power = 0;
  this->ram_readings_.type = static_cast<uint>(ObserverType::NONE);
  this->ram_readings_.timestamp = 0;
  this->ram_readings_.difference = 0;
  return Status{};
}

uint64_t ProcMemInfoObserver::GetUptime() {
  float uptime = 0.f;
  float uptime_idle = 0.f;
  FILE *proc_uptime_file = fopen("/proc/uptime", "r");

  if (proc_uptime_file == NULL) {
    return 0;
  }

  fscanf(proc_uptime_file, "%f %f", &uptime, &uptime_idle);
  fclose(proc_uptime_file);

  this->uptime_ = static_cast<uint64_t>(uptime * sysconf(_SC_CLK_TCK)) * 10;

  return this->uptime_;
}

void ProcMemInfoObserver::GetProcMemInfo() {
  std::string filename{"/proc/meminfo"};
  std::ifstream fs{filename};

  std::vector<std::string> values;
  std::string intermediate, line;

  /* Read the file */
  while (std::getline(fs, line)) {
    /* Get every line for parsing */
    std::istringstream linestream(line);
    while (std::getline(linestream, intermediate, ' ')) {
      values.push_back(intermediate);
    }

    /* Analyse the tokens */
    std::size_t sec_lindex = values.size() - 2;
    if (values[0].find("MemAvailable") != std::string::npos) {
      this->proc_data_.phys_available = std::stoull(values[sec_lindex]);
    } else if (values[0].find("SwapFree") != std::string::npos) {
      this->proc_data_.swap_available = std::stoull(values[sec_lindex]);
    } else if (values[0].find("MemTotal") != std::string::npos) {
      this->proc_data_.phys_total = std::stoull(values[sec_lindex]);
    } else if (values[0].find("SwapTotal") != std::string::npos) {
      this->proc_data_.swap_total = std::stoull(values[sec_lindex]);
    }

    /* Clear to about crowding */
    values.clear();
  }
}

void ProcMemInfoObserver::TranslateReadings() noexcept {
  /* Base object */
  this->ram_readings_.type = static_cast<int>(ObserverType::IO);
  this->ram_readings_.difference =
      this->uptime_ - this->ram_readings_.timestamp;
  this->ram_readings_.timestamp = this->uptime_;

  /* Compute the RAM consumption. Factor 10 converts from KiB to MiB */
  this->ram_readings_.type = static_cast<int>(ObserverType::RAM);
  this->ram_readings_.overall_usage =
      (this->proc_data_.phys_total - this->proc_data_.phys_available) >> 10;
  this->ram_readings_.swap_usage =
      (this->proc_data_.swap_total - this->proc_data_.swap_available) >> 10;
  this->ram_readings_.total_memory_usage =
      this->ram_readings_.overall_usage + this->ram_readings_.swap_usage;

  /* Not supported metrics */
  this->ram_readings_.overall_power = -1.f;
  this->ram_readings_.overall_bw = -1.f;
}
} /* namespace efimon */
