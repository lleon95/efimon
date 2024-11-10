/**
 * @file ptrace-capstone.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Ptrace-capstone-based ASM low-weight extractor
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/ptrace-capstone/ptrace-capstone.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/instruction-readings.hpp>
#include <efimon/status.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace efimon {

extern uint64_t GetUptime();

PTraceCapstoneObserver::PTraceCapstoneObserver(const uint pid,
                                               const ObserverScope scope,
                                               const uint64_t interval)
    : Observer{}, readings_{}, valid_{false} {
  this->pid_ = pid;
  this->interval_ = interval;
  uint64_t type = static_cast<uint64_t>(ObserverType::CPU) |
                  static_cast<uint64_t>(ObserverType::INTERVAL) |
                  static_cast<uint64_t>(ObserverType::CPU_INSTRUCTIONS);

  if (ObserverScope::PROCESS != scope) {
    throw Status{Status::INVALID_PARAMETER, "System-scope is not supported"};
  }

  this->caps_.emplace_back();
  this->caps_[0].type = type;

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || \
    defined(__i386__) || defined(__i386) || defined(_M_IX86)
  this->classifier_ = AsmClassifier::Build(assembly::Architecture::X86);
#else
  this->classifier_ = nullptr;
#endif
}

Status PTraceCapstoneObserver::Trigger() {
  Status ret{};

  return ret;
}

std::vector<Readings*> PTraceCapstoneObserver::GetReadings() {
  return std::vector<Readings*>{static_cast<Readings*>(&(this->readings_))};
}

Status PTraceCapstoneObserver::SelectDevice(const uint /* device */) {
  return Status{
      Status::NOT_IMPLEMENTED,
      "It is not possible to select a device since this is a wrapper class"};
}

Status PTraceCapstoneObserver::SetScope(const ObserverScope /* scope */) {
  return Status{
      Status::NOT_IMPLEMENTED,
      "It is not possible change the scope since this is a wrapper class"};
}

Status PTraceCapstoneObserver::SetPID(const uint pid) {
  this->pid_ = pid;
  return Status{};
}

ObserverScope PTraceCapstoneObserver::GetScope() const noexcept {
  return ObserverScope::PROCESS;
}

uint PTraceCapstoneObserver::GetPID() const noexcept { return this->pid_; }

const std::vector<ObserverCapabilities>&
PTraceCapstoneObserver::GetCapabilities() const noexcept {
  return this->caps_;
}

Status PTraceCapstoneObserver::GetStatus() { return Status{}; }

Status PTraceCapstoneObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status PTraceCapstoneObserver::ClearInterval() {
  this->interval_ = 0;
  return Status{Status::OK, "The clear interval has been cleared"};
}

Status PTraceCapstoneObserver::Reset() {
  this->readings_.timestamp = 0;
  this->readings_.difference = 0;
  this->valid_ = false;
  this->readings_.type = static_cast<int>(ObserverType::CPU);
  return Status{};
}

PTraceCapstoneObserver::~PTraceCapstoneObserver() {}

} /* namespace efimon */
