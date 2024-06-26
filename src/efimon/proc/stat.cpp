/**
 * @file stat.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the /proc/pid/stat looking for metrics about RAM and CPU
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <efimon/proc/stat.hpp>
#include <fstream>
#include <mutex>  // NOLINT
#include <numeric>
#include <sstream>
#include <vector>
extern std::mutex m_single_uptime;

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
                  static_cast<uint64_t>(ObserverType::INTERVAL);
  this->pid_ = pid;
  this->interval_ = interval;
  this->status_ = Status{};
  this->caps_.emplace_back();
  this->caps_[0].type = type;
  if (ObserverScope::SYSTEM == scope) {
    this->global_ = true;
  } else {
    this->global_ = false;
    type |= static_cast<uint64_t>(ObserverType::RAM);
  }
  this->caps_[0].scope = scope;
  this->Reset();
}

ProcStatObserver::~ProcStatObserver() {}

Status ProcStatObserver::Trigger() {
  /* Check if the process is alive */
  if (!this->global_) {
    CheckAlive();
    if (Status::OK != this->status_.code) {
      return status_;
    }
  }

  /* Update the uptime */
  GetUptime();

  /* Get the ProcStat and process the results */
  if (!this->global_) {
    GetProcStat();
    TranslateReadings();
  } else {
    GetGlobalProcStat();
    TranslateGlobalReadings();
  }

  return Status{};
}

std::vector<Readings *> ProcStatObserver::GetReadings() {
  std::vector<Readings *> readings{};
  readings.push_back(&this->cpu_readings_);
  if (!this->global_) readings.push_back(&this->ram_readings_);
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

Status ProcStatObserver::SetPID(const uint pid) {
  this->pid_ = pid;
  return this->Reset();
}

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
  std::memset(&this->proc_global_data_, 0,
              MAX_NUM_CPUS * sizeof(ProcStatGlobalData));
  this->cpu_readings_.type = static_cast<uint>(ObserverType::NONE);
  this->cpu_readings_.timestamp = 0;
  this->cpu_readings_.difference = 0;
  this->cpu_readings_.overall_usage = 0;
  this->cpu_readings_.overall_power = 0;
  this->cpu_readings_.core_usage.clear();
  this->cpu_readings_.core_power.clear();
  this->ram_readings_.overall_usage = 0;
  this->ram_readings_.overall_bw = 0;
  this->ram_readings_.overall_power = 0;
  this->ram_readings_.type = static_cast<uint>(ObserverType::NONE);
  this->ram_readings_.timestamp = 0;
  this->ram_readings_.difference = 0;
  return Status{};
}

uint64_t ProcStatObserver::GetUptime() {
  std::scoped_lock lock(m_single_uptime);
  float uptime = 0.f;
  float uptime_idle = 0.f;
  FILE *proc_uptime_file = fopen("/proc/uptime", "r");

  if (proc_uptime_file == NULL) {
    return 0;
  }

  fscanf(proc_uptime_file, "%f %f", &uptime, &uptime_idle);
  fclose(proc_uptime_file);

  this->uptime_ = static_cast<uint64_t>(uptime * sysconf(_SC_CLK_TCK)) * 10;
  this->uptime_idle_ =
      static_cast<uint64_t>(uptime_idle * sysconf(_SC_CLK_TCK)) * 10;

  return this->uptime_;
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
         "%*u %*u %lu %lu %ld %ld %*d %*d %*d "
         "%*d %lu %lu %ld %*u %*p %*p %*p %*p "
         "%*p %*u %*u %*u %*u %*p %*u %*u %*d %d "
         "%*u %*u %*u %*u %*d %*p %*p %*p %*p %*p %*p %*p %*d",
         &ps->pid, &ps->state, &ps->utime, &ps->stime, &ps->cutime, &ps->cstime,
         &ps->starttime, &ps->vsize, &ps->rss, &ps->processor);

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
  this->cpu_readings_.type = static_cast<int>(ObserverType::CPU);
  this->ram_readings_.type = static_cast<int>(ObserverType::RAM);
  this->cpu_readings_.difference =
      this->uptime_ - this->cpu_readings_.timestamp;
  this->cpu_readings_.timestamp = this->uptime_;
  this->ram_readings_.difference = this->cpu_readings_.difference;
  this->ram_readings_.timestamp = this->cpu_readings_.timestamp;

  /* CPU-specific */
  uint64_t total = this->uptime_ - this->proc_data_.starttime;
  uint64_t active = (this->proc_data_.utime + this->proc_data_.stime +
                     this->proc_data_.cutime + this->proc_data_.cstime);
  active *= 1000;
  active /= sysconf(_SC_CLK_TCK);

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
  this->cpu_readings_.overall_usage =
      warmup ? 0.f : total_usage / (float)total_processors;  // NOLINT

  /* Compute the RAM consumption. Factor 20 is MiB */
  this->ram_readings_.overall_usage =
      proc_data_.rss * sysconf(_SC_PAGE_SIZE) >> 20;
  this->ram_readings_.total_memory_usage = proc_data_.vsize >> 20;
  this->ram_readings_.swap_usage = this->ram_readings_.total_memory_usage -
                                   this->ram_readings_.overall_usage;

  /* Not supported metrics */
  this->cpu_readings_.overall_power = -1.f;
  this->ram_readings_.overall_power = -1.f;
  this->ram_readings_.overall_bw = -1.f;
  this->cpu_readings_.core_power.resize(total_processors);
  this->cpu_readings_.core_usage.resize(total_processors);
  std::fill(this->cpu_readings_.core_power.begin(),
            this->cpu_readings_.core_power.end(), -1.f);
  std::fill(this->cpu_readings_.core_usage.begin(),
            this->cpu_readings_.core_usage.end(), -1.f);
}

void ProcStatObserver::GetGlobalProcStat() {
  const uint32_t total_processors = sysconf(_SC_NPROCESSORS_ONLN);

  std::string filename{"/proc/stat"};
  std::ifstream fs{filename};

  std::vector<std::string> values;
  std::string intermediate, line;

  /* Read the document */
  for (uint32_t i = 0; (i <= total_processors) && (i < MAX_NUM_CPUS); ++i) {
    /* Get every line */
    std::getline(fs, line);
    std::istringstream linestream(line);
    /* Get each value from the line */
    while (std::getline(linestream, intermediate, ' ')) {
      values.push_back(intermediate);
    }

    /* Analyse the cases
       Based on: https://man7.org/linux/man-pages/man5/proc.5.html */
    uint32_t offset = 0 == i ? 2 : 1; /* Position of the first reading */
    this->proc_global_data_[i].user = std::stoull(values.at(offset++));
    this->proc_global_data_[i].nice = std::stoull(values.at(offset++));
    this->proc_global_data_[i].system = std::stoull(values.at(offset++));
    this->proc_global_data_[i].idle = std::stoull(values.at(offset++));
    this->proc_global_data_[i].iowait = std::stoull(values.at(offset++));
    this->proc_global_data_[i].cpu_idx = i - 1;

    values.clear();
  }
}

void ProcStatObserver::TranslateGlobalReadings() noexcept {
  bool warmup = false;
  uint32_t total_processors = sysconf(_SC_NPROCESSORS_ONLN);
  uint64_t total_global_time = 0;

  /* Base object */
  this->cpu_readings_.type = static_cast<int>(ObserverType::CPU);
  this->cpu_readings_.difference =
      this->uptime_ - this->cpu_readings_.timestamp;
  this->cpu_readings_.timestamp = this->uptime_;

  /* Prepare vectors */
  this->cpu_readings_.core_power.resize(total_processors);
  this->cpu_readings_.core_usage.resize(total_processors);

  /* CPU-specific */
  for (uint32_t i = 0; (i <= total_processors) && (i < MAX_NUM_CPUS); ++i) {
    /* Compute times for each core. The first is the total */
    uint64_t total_active_time_ms =
        (this->proc_global_data_[i].user + this->proc_global_data_[i].nice +
         this->proc_global_data_[i].system + this->proc_global_data_[i].iowait +
         this->proc_global_data_[i].idle * 0.01);
    total_active_time_ms = total_active_time_ms * 100000 / sysconf(_SC_CLK_TCK);
    uint64_t diff_total_time = this->uptime_ - this->proc_global_data_[i].total;
    uint64_t diff_active_time =
        total_active_time_ms - this->proc_global_data_[i].active;
    this->proc_global_data_[i].total = this->uptime_;
    this->proc_global_data_[i].active = total_active_time_ms;

    /* Compute the percentage */
    float total_usage =
        ((float)diff_active_time / (float)diff_total_time);  // NOLINT

    if (0 == i) {
      total_global_time = total_usage;
      continue;
    }

    /* Document the percentage per core*/
    this->cpu_readings_.core_usage[i - 1] = total_usage;
  }

  this->cpu_readings_.overall_usage =
      warmup ? 0.f : total_global_time / (float)total_processors;  // NOLINT

  /* Not supported metrics */
  this->cpu_readings_.overall_power = -1.f;
  std::fill(this->cpu_readings_.core_power.begin(),
            this->cpu_readings_.core_power.end(), -1.f);
}

} /* namespace efimon */
