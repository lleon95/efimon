/**
 * @file stat.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the /proc/pid/stat looking for metrics about RAM and CPU
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <efimon/proc/net.hpp>
#include <efimon/status.hpp>

#include <unistd.h>

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <iostream>

#define MAX_LEN_FILE_PATH 255

namespace efimon {
ProcNetObserver::ProcNetObserver(const uint /*pid*/, const ObserverScope scope,
                                 const uint64_t interval)
    : Observer{} {
  uint64_t type = static_cast<uint64_t>(ObserverType::NETWORK) |
                  static_cast<uint64_t>(ObserverType::INTERVAL);
  this->pid_ = -1;
  this->device_ = 0;
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

ProcNetObserver::~ProcNetObserver() {}

Status ProcNetObserver::Trigger() {
  /* Update the uptime */
  GetUptime();

  /* Get the ProcMemInfo and process the results */
  GetProcNetDev();

  return Status{};
}

std::vector<Readings *> ProcNetObserver::GetReadings() {
  std::vector<Readings *> readings{};
  for (auto &reading : this->net_readings_) {
    readings.push_back(&reading);
  }
  return readings;
}

Status ProcNetObserver::SelectDevice(const uint device) {
  this->device_ = device;
  return Status{};
}

Status ProcNetObserver::SetScope(const ObserverScope /*scope*/) {
  return Status{Status::NOT_IMPLEMENTED,
                "Cannot select a device since it is not implemented"};
}

Status ProcNetObserver::SetPID(const uint /*pid*/) {
  return Status{Status::NOT_IMPLEMENTED,
                "Cannot set a PID since it is not implemented"};
}

ObserverScope ProcNetObserver::GetScope() const noexcept {
  return this->caps_[0].scope;
}

uint ProcNetObserver::GetPID() const noexcept { return this->pid_; }

const std::vector<ObserverCapabilities> &ProcNetObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status ProcNetObserver::GetStatus() { return this->status_; }

Status ProcNetObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status ProcNetObserver::ClearInterval() { return Status{}; }

Status ProcNetObserver::Reset() {
  /* Resetting the structures is more than enough */
  this->net_readings_.clear();
  return Status{};
}

uint64_t ProcNetObserver::GetUptime() {
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

void ProcNetObserver::GetProcNetDev() {
  std::string filename{"/proc/net/dev"};
  std::ifstream fs{filename};

  std::vector<std::string> values;
  std::string intermediate, line;
  std::vector<NetReadings> net_readings_loc;

  /* Base object */
  int type = static_cast<int>(ObserverType::NETWORK);
  uint64_t difference = this->uptime_ - this->prev_uptime_;
  this->prev_uptime_ = this->uptime_;

  /* Read the file */
  int lines = 0;
  while (std::getline(fs, line)) {
    if (1 >= lines++) continue; /* Skip first lines */
    /* Get every line for parsing */
    std::istringstream linestream(line);
    while (std::getline(linestream, intermediate, ' ')) {
      if (intermediate != " " && intermediate != "") {
        values.push_back(intermediate);
      }
    }

    std::string devname = values.at(0).substr(0, values.at(0).size() - 1);
    float overall_tx_volume =
        static_cast<float>(std::stoull(values.at(9))) / 1024.f;
    float overall_rx_volume =
        static_cast<float>(std::stoull(values.at(1))) / 1024.f;
    float overall_tx_bw = 0.f;
    float overall_rx_bw = 0.f;

    bool init = this->data_.find(devname) == this->data_.end();
    if (!init) {
      overall_tx_bw =
          (overall_tx_volume - this->data_[devname].overall_tx_volume) /
          difference;
      overall_rx_bw =
          (overall_rx_volume - this->data_[devname].overall_rx_volume) /
          difference;
    } else {
      this->data_[devname] = NetReadings{};
    }

    this->data_[devname].type = type;
    this->data_[devname].timestamp = this->uptime_;
    this->data_[devname].difference = difference;

    this->data_[devname].overall_tx_bw = overall_tx_bw * 1000.f;
    this->data_[devname].overall_rx_bw = overall_rx_bw * 1000.f;
    this->data_[devname].overall_tx_volume = overall_tx_volume;
    this->data_[devname].overall_rx_volume = overall_rx_volume;
    this->data_[devname].overall_tx_packets = std::stoull(values.at(10));
    this->data_[devname].overall_rx_packets = std::stoull(values.at(2));

    this->data_[devname].dev_name =
        values.at(0).substr(0, values.at(0).size() - 1);

    /* Clear to about crowding */
    values.clear();
  }

  this->net_readings_.clear();
  for (auto kv : this->data_) {
    this->device_names_.emplace_back(kv.first);
    this->net_readings_.emplace_back(kv.second);
  }
}

const std::vector<std::string> &ProcNetObserver::GetDeviceNames() const
    noexcept {
  return this->device_names_;
}

} /* namespace efimon */
