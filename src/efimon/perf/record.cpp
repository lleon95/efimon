/**
 * @file record.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer to query the perf record command. It works as a wrapper
 * for other functions like perf annotate
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <cstdlib>
#include <efimon/perf/record.hpp>
#include <filesystem>
#include <iostream>
#include <string>

namespace efimon {

extern uint64_t GetUptime();

PerfRecordObserver::PerfRecordObserver(const uint pid,
                                       const ObserverScope /* scope */,
                                       const uint64_t interval,
                                       const uint64_t frequency,
                                       const bool no_dispose)
    : Observer{}, valid_{false}, frequency_{frequency} {
  this->pid_ = pid;
  this->interval_ = interval;
  this->no_dispose_ = no_dispose;
  if (pid == 0) return;
  if (interval == 0) interval_ = 1000;
  if (frequency == 0) frequency_ = 1000;

  this->CreateTemporaryFolder();
  this->MakePerfCommand();
}

void PerfRecordObserver::CreateTemporaryFolder() {
  tmp_folder_path_ = std::filesystem::temp_directory_path() /
                     ("efimon-" + std::to_string(pid_));
  std::filesystem::create_directory(tmp_folder_path_);
}

void PerfRecordObserver::MakePerfCommand() {
  this->perf_cmd_ += "cd " + std::string(this->tmp_folder_path_);
  this->perf_cmd_ += " && perf record -q -F";
  this->perf_cmd_ += std::to_string(this->frequency_);
  this->perf_cmd_ += " -g -v -p ";
  this->perf_cmd_ += std::to_string(this->pid_);
  this->perf_cmd_ += " -a sleep ";
  this->perf_cmd_ += std::to_string(this->interval_);
}

void PerfRecordObserver::MovePerfData(const std::filesystem::path& ipath,
                                      const std::filesystem::path& opath) {
  std::filesystem::copy_file(ipath, opath,
                             std::filesystem::copy_options::update_existing);
  this->valid_ = true;
  this->path_to_perf_data_ = opath;
}

void PerfRecordObserver::DisposeTemporaryFolder() {
  if (!this->no_dispose_) std::filesystem::remove_all(tmp_folder_path_);
}

Status PerfRecordObserver::Trigger() {
  auto target_path = this->tmp_folder_path_ / "perf.data.ulock";
  std::system(this->perf_cmd_.c_str());
  this->MovePerfData(this->tmp_folder_path_ / "perf.data", target_path);

  auto time = GetUptime();
  this->readings_.perf_data_path = std::string(this->path_to_perf_data_);
  this->readings_.type = static_cast<uint64_t>(ObserverType::CPU);
  this->readings_.difference = time - this->readings_.timestamp;
  this->readings_.timestamp = time;
  return Status{};
}

std::vector<Readings*> PerfRecordObserver::GetReadings() {
  return std::vector<Readings*>{static_cast<Readings*>(&(this->readings_))};
}

Status PerfRecordObserver::SelectDevice(const uint /* device */) {
  return Status{Status::NOT_IMPLEMENTED, "Cannot select a device"};
}

Status PerfRecordObserver::SetScope(const ObserverScope scope) {
  if (ObserverScope::PROCESS == scope) return Status{};
  return Status{Status::NOT_IMPLEMENTED, "The scope is only set to PROCESS"};
}

Status PerfRecordObserver::SetPID(const uint pid) {
  this->pid_ = pid;
  return Status{};
}

ObserverScope PerfRecordObserver::GetScope() const noexcept {
  return ObserverScope::PROCESS;
}

uint PerfRecordObserver::GetPID() const noexcept { return this->pid_; }

const std::vector<ObserverCapabilities>& PerfRecordObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status PerfRecordObserver::GetStatus() { return Status{}; }

Status PerfRecordObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status PerfRecordObserver::ClearInterval() {
  return Status{Status::NOT_IMPLEMENTED,
                "The clear interval is not implemented yet"};
}

Status PerfRecordObserver::Reset() {
  this->readings_.perf_data_path = "";
  this->readings_.type = static_cast<uint>(ObserverType::NONE);
  this->readings_.timestamp = 0;
  this->readings_.difference = 0;
  return Status{};
}

PerfRecordObserver::~PerfRecordObserver() { this->DisposeTemporaryFolder(); }
} /* namespace efimon */
