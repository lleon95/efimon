/**
 * @file io.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the /proc/pid/io looking for metrics about I/O usage
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <efimon/proc/io.hpp>
#include <mutex>  // NOLINT
#include <numeric>
#include <vector>
extern std::mutex m_single_uptime;

#define MAX_LEN_FILE_PATH 255

namespace efimon {
ProcIOObserver::ProcIOObserver(const uint pid, const ObserverScope scope,
                               const uint64_t interval)
    : Observer{} {
  uint64_t type = static_cast<uint64_t>(ObserverType::IO) |
                  static_cast<uint64_t>(ObserverType::INTERVAL);
  this->pid_ = pid;
  this->interval_ = interval;
  this->status_ = Status{};
  this->caps_.emplace_back();
  this->caps_[0].type = type;

  if (ObserverScope::SYSTEM == scope) {
    throw Status{Status::NOT_IMPLEMENTED, "System monitor not implemented"};
  }
  this->caps_[0].scope = scope;
  this->Reset();
}

ProcIOObserver::~ProcIOObserver() {}

Status ProcIOObserver::Trigger() {
  /* Check if the process is alive */
  CheckAlive();
  if (Status::OK != this->status_.code) {
    return status_;
  }

  /* Update the uptime */
  this->uptime_ = GetUptime();

  /* Get the ProcIO and process the results */
  GetProcIO();
  TranslateReadings();

  return Status{};
}

std::vector<Readings *> ProcIOObserver::GetReadings() {
  std::vector<Readings *> readings{};
  readings.push_back(&this->io_readings_);
  return readings;
}

Status ProcIOObserver::SelectDevice(const uint /*device*/) {
  return Status{Status::NOT_IMPLEMENTED,
                "Cannot select a device since it is not implemented"};
}

Status ProcIOObserver::SetScope(const ObserverScope /*scope*/) {
  return Status{Status::NOT_IMPLEMENTED,
                "Cannot select a device since it is not implemented"};
}

Status ProcIOObserver::SetPID(const uint /*pid*/) { return Status{}; }

ObserverScope ProcIOObserver::GetScope() const noexcept {
  return this->caps_[0].scope;
}

uint ProcIOObserver::GetPID() const noexcept { return this->pid_; }

const std::vector<ObserverCapabilities> &ProcIOObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status ProcIOObserver::GetStatus() { return this->status_; }

Status ProcIOObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status ProcIOObserver::ClearInterval() { return Status{}; }

Status ProcIOObserver::Reset() {
  /* Resetting the structures is more than enough */
  std::memset(&this->proc_data_, 0, sizeof(ProcIOData));
  this->io_readings_.type = static_cast<uint>(ObserverType::NONE);
  this->io_readings_.timestamp = 0;
  this->io_readings_.difference = 0;
  this->io_readings_.read_bw = -1;
  this->io_readings_.write_bw = -1;
  this->io_readings_.read_volume = -1;
  this->io_readings_.write_volume = -1;
  this->io_readings_.read_volume = -1;
  this->io_readings_.write_volume = -1;

  return Status{};
}

uint64_t ProcIOObserver::GetUptime() {
  std::scoped_lock lock(m_single_uptime);
  float uptime = 0.f;
  FILE *proc_uptime_file = fopen("/proc/uptime", "r");

  if (proc_uptime_file == NULL) {
    return 0;
  }

  fscanf(proc_uptime_file, "%f", &uptime);
  fclose(proc_uptime_file);

  return static_cast<uint64_t>(uptime * sysconf(_SC_CLK_TCK)) * 10;
}

void ProcIOObserver::GetProcIO() {
  char path[MAX_LEN_FILE_PATH] = {0};
  FILE *procfp = NULL;
  ProcIOData *ps = &this->proc_data_;

  /* Prepare to open the file */
  snprintf(path, MAX_LEN_FILE_PATH, "/proc/%i/io", this->pid_);

  procfp = fopen(path, "r");

  if (procfp == NULL) {
    this->status_ = Status{Status::NOT_FOUND, "The process is not available"};
    return;
  }

  fscanf(procfp, "%*s %lu %*s %lu ", &ps->rchar, &ps->wchar);
  fclose(procfp);
}

void ProcIOObserver::CheckAlive() {
  char path[MAX_LEN_FILE_PATH] = {0};
  FILE *procfp = NULL;

  /* Prepare to open the file */
  snprintf(path, MAX_LEN_FILE_PATH, "/proc/%i/io", this->pid_);

  procfp = fopen(path, "r");

  if (procfp == NULL) {
    this->status_ = Status{Status::NOT_FOUND, "The process is not available"};
  } else {
    this->status_ = Status{};
    fclose(procfp);
  }
}

void ProcIOObserver::TranslateReadings() noexcept {
  /* Base object */
  this->io_readings_.type = static_cast<int>(ObserverType::IO);
  this->io_readings_.difference = this->uptime_ - this->io_readings_.timestamp;
  this->io_readings_.timestamp = this->uptime_;

  /* IO difference */
  uint64_t rchar = this->proc_data_.rchar - this->io_readings_.read_volume;
  uint64_t wchar = this->proc_data_.wchar - this->io_readings_.write_volume;

  /* IO total volume */
  this->io_readings_.read_volume = this->proc_data_.rchar;
  this->io_readings_.write_volume = this->proc_data_.wchar;

  /* Bandwidth. Factor of 1000 to convert from ms to s */
  this->io_readings_.read_bw =
      1000 * static_cast<float>(rchar) /
      static_cast<float>(this->io_readings_.difference);
  this->io_readings_.write_bw =
      1000 * static_cast<float>(wchar) /
      static_cast<float>(this->io_readings_.difference);

  /* Invalid - not supported */
  this->io_readings_.read_power = -1.f;
  this->io_readings_.write_power = -1.f;
}
} /* namespace efimon */
