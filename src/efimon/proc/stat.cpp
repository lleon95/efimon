/**
 * @file stat.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the /proc/pid/stat looking for metrics about RAM and CPU
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <efimon/proc/stat.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <numeric>
#include <vector>

#define MAX_LEN_FILE_PATH 255

/**
 * @brief Enumerates the process states
 * The process can be detected in multiple states and this enum synthesises
 * those possible states.
 */
enum ProcState {
  /** Running state */
  PROC_STATE_RUNNING = 'R',
  /** Sleeping in interrumpible state*/
  PROC_STATE_INT_SLEEP = 'S',
  /** Disk sleep (not interrumpible) */
  PROC_STATE_DISK_SLEEP = 'D',
  /** Zombie detached state */
  PROC_STATE_ZOMBIE = 'Z',
  /** Stopped in a trap or on a signal */
  PROC_STATE_STOPPED = 'T',
  /** Dead */
  PROC_STATE_DEAD = 'X',
};

namespace efimon {
ProcStatObserver::ProcStatObserver(const uint pid, const ObserverScope scope,
                                   const uint64_t interval)
    : Observer{} {
  uint64_t type = static_cast<uint64_t>(ObserverType::CPU) |
                  static_cast<uint64_t>(ObserverType::RAM) |
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

ProcStatObserver::~ProcStatObserver() {}

Status ProcStatObserver::Trigger() {
  /* Check if the process is alive */
  CheckAlive();
  if (Status::OK != this->status_.code) {
    return status_;
  }

  /* Update the uptime */
  this->uptime_ = GetUptime();

  /* Get the ProcStat and process the results */
  GetProcStat();
  TranslateReadings();

  return Status{};
}

std::vector<Readings *> ProcStatObserver::GetReadings() {
  std::vector<Readings *> readings{};
  readings.push_back(&this->readings_);
  return readings;
}

Status ProcStatObserver::SelectDevice(const uint /*device*/) {
  return Status{Status::NOT_IMPLEMENTED,
                "Cannot select a device since it is not implemented"};
}

Status ProcStatObserver::SetScope(const ObserverScope /*scope*/) {
  return Status{Status::NOT_IMPLEMENTED,
                "Cannot select a device since it is not implemented"};
}

Status ProcStatObserver::SetPID(const uint /*pid*/) { return Status{}; }

ObserverScope ProcStatObserver::GetScope() const noexcept {
  return this->caps_[0].scope;
}

uint ProcStatObserver::GetPID() const noexcept { return this->pid_; }

const std::vector<ObserverCapabilities> &ProcStatObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status ProcStatObserver::GetStatus() { return this->status_; }

Status ProcStatObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status ProcStatObserver::ClearInterval() { return Status{}; }

Status ProcStatObserver::Reset() {
  /* Resetting the structures is more than enough */
  std::memset(&this->proc_data_, 0, sizeof(ProcStatData));
  this->readings_.type = static_cast<uint>(ObserverType::NONE);
  this->readings_.timestamp = 0;
  this->readings_.difference = 0;
  this->readings_.overall_usage = 0;
  this->readings_.overall_power = 0;
  this->readings_.core_usage.clear();
  this->readings_.core_power.clear();
  return Status{};
}

uint64_t ProcStatObserver::GetUptime() {
  float uptime = 0.f;
  FILE *proc_uptime_file = fopen("/proc/uptime", "r");

  if (proc_uptime_file == NULL) {
    return 0;
  }

  fscanf(proc_uptime_file, "%f", &uptime);
  fclose(proc_uptime_file);

  return static_cast<uint64_t>(uptime * sysconf(_SC_CLK_TCK));
}

void ProcStatObserver::GetProcStat() {
  char path[MAX_LEN_FILE_PATH] = {0};
  FILE *procfp = NULL;
  ProcStatData *ps = &this->proc_data_;

  /* Prepare to open the file */
  snprintf(path, MAX_LEN_FILE_PATH, "/proc/%u/stat", this->pid_);

  procfp = fopen(path, "r");

  if (procfp == NULL) {
    this->status_ = Status{Status::NOT_FOUND, "The process is not available"};
    return;
  }

  fscanf(procfp,
         "%d %*s %c %*d %*d %*d %*d %*d %*u %*u %*u "
         "%*u %*u %lu %lu %*d %*d %*d %*d %*d "
         "%*d %lu %lu %ld %*u %*p %*p %*p %*p "
         "%*p %*u %*u %*u %*u %*p %*u %*u %*d %d "
         "%*u %*u %*u %*u %*d %*p %*p %*p %*p %*p %*p %*p %*d",
         &ps->pid, &ps->state, &ps->utime, &ps->stime, &ps->starttime,
         &ps->vsize, &ps->rss, &ps->processor);

  fclose(procfp);
}

void ProcStatObserver::CheckAlive() {
  char path[MAX_LEN_FILE_PATH] = {0};
  FILE *procfp = NULL;

  /* Prepare to open the file */
  snprintf(path, MAX_LEN_FILE_PATH, "/proc/%i/stat", this->pid_);

  procfp = fopen(path, "r");

  if (procfp == NULL) {
    this->status_ = Status{Status::NOT_FOUND, "The process is not available"};
  } else {
    this->status_ = Status{};
    fclose(procfp);
  }
}

void ProcStatObserver::TranslateReadings() noexcept {
  bool warmup = false;

  /* Base object */
  this->readings_.type = this->caps_[0].type;
  this->readings_.difference = this->uptime_ - this->readings_.timestamp;
  this->readings_.timestamp = this->uptime_;

  /* CPU-specific */
  uint64_t total = this->uptime_ - this->proc_data_.starttime;
  uint64_t active = this->proc_data_.utime + this->proc_data_.stime;

  if (PROC_STATE_STOPPED == this->proc_data_.state ||
      PROC_STATE_DEAD == this->proc_data_.state) {
    /* Invalid. Return */
    return;
  }

  uint32_t total_processors = sysconf(_SC_NPROCESSORS_ONLN);

  uint64_t diff_total = total - this->proc_data_.total;
  uint64_t diff_active = active - this->proc_data_.active;

  warmup = 0u == this->proc_data_.total || 0u == diff_total;

  /* Update the structure */
  this->proc_data_.total = total;
  this->proc_data_.active = active;

  /* Compute the percentage */
  float total_usage =
      100.f * ((float)diff_active / (float)diff_total);  // NOLINT
  this->readings_.overall_usage = warmup ? 0.f : total_usage / total_processors;

  /* Not supported metrics */
  this->readings_.overall_power = -1.f;
  this->readings_.core_power.resize(total_processors);
  this->readings_.core_usage.resize(total_processors);
  std::fill(this->readings_.core_power.begin(),
            this->readings_.core_power.end(), -1.f);
  std::fill(this->readings_.core_usage.begin(),
            this->readings_.core_usage.end(), -1.f);
}

} /* namespace efimon */
