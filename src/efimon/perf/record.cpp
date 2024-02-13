/**
 * @file record.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer to query the perf record command. It works as a wrapper
 * for other functions like perf annotate
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/perf/record.hpp>

#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace efimon {

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
  this->perf_cmd_ += " &> ";
  this->perf_cmd_ +=
      std::string(this->tmp_folder_path_ / std::string("perf.log"));
}

void PerfRecordObserver::MovePerfData(const std::filesystem::path& ipath,
                                      const std::filesystem::path& opath) {
  std::filesystem::copy_file(ipath, opath);
}

void PerfRecordObserver::DisposeTemporaryFolder() {
  if (!this->no_dispose_) std::filesystem::remove_all(tmp_folder_path_);
}

Status PerfRecordObserver::Trigger() {
  std::cout << this->perf_cmd_ << std::endl;
  std::system(this->perf_cmd_.c_str());
  sleep(this->interval_ + 1);
  this->MovePerfData(this->tmp_folder_path_ / "perf.data",
                     this->tmp_folder_path_ / "perf.data.ulock");
  return Status{};
}

std::vector<Readings*> PerfRecordObserver::GetReadings() {
  return std::vector<Readings*>{};
}

Status PerfRecordObserver::SelectDevice(const uint /* device */) {
  return Status{};
}

Status PerfRecordObserver::SetScope(const ObserverScope /* scope */) {
  return Status{};
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

Status PerfRecordObserver::ClearInterval() { return Status{}; }

Status PerfRecordObserver::Reset() { return Status{}; }

PerfRecordObserver::~PerfRecordObserver() { this->DisposeTemporaryFolder(); }
} /* namespace efimon */
