/**
 * @file stat.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the /proc/pid/stat looking for metrics about RAM and CPU
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <efimon/proc/stat.hpp>

namespace efimon {
ProcStatObserver::ProcStatObserver(const uint pid, const ObserverScope scope,
                                   const uint64_t interval)
    : Observer{} {
  uint64_t type = static_cast<uint64_t>(ObserverType::CPU) |
                  static_cast<uint64_t>(ObserverType::RAM) |
                  static_cast<uint64_t>(ObserverType::INTERVAL);
  pid_ = pid;
  interval_ = interval;
  status_ = Status{};
  caps_.emplace_back();
  caps_[0].type = type;
  caps_[0].scope = scope;
}

ProcStatObserver::~ProcStatObserver() {}

Status ProcStatObserver::Trigger() { return Status{}; }

std::vector<Readings> ProcStatObserver::GetReadings() {
  return std::vector<Readings>{};
}

Status ProcStatObserver::SelectDevice(const uint /*device*/) {
  return Status{};
}

Status ProcStatObserver::SetScope(const ObserverScope /*scope*/) {
  return Status{};
}

Status ProcStatObserver::SetPID(const uint /*pid*/) { return Status{}; }

ObserverScope ProcStatObserver::GetScope() const noexcept {
  return caps_[0].scope;
}

uint ProcStatObserver::GetPID() const noexcept { return pid_; }

const std::vector<ObserverCapabilities>& ProcStatObserver::GetCapabilities()
    const noexcept {
  return caps_;
}

Status ProcStatObserver::GetStatus() { return Status{}; }

Status ProcStatObserver::SetInterval(const uint64_t interval) {
  interval_ = interval;
  return Status{};
}

Status ProcStatObserver::ClearInterval() { return Status{}; }

Status ProcStatObserver::Reset() { return Status{}; }

} /* namespace efimon */
