/**
 * @file annotate.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer to query the perf annotate command. This depends on the
 * perf record class
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <cstdlib>
#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/perf/annotate.hpp>
#include <efimon/readings.hpp>
#include <efimon/status.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifndef PERF_ANNOTATE_THRES
#define PERF_ANNOTATE_THRES 0.01
#endif /* PERF_ANNOTATE_THRES */

namespace efimon {

PerfAnnotateObserver::PerfAnnotateObserver(PerfRecordObserver& record)
    : Observer{}, record_{record} {
  const std::string filename = "annotation.txt";
  this->valid_ = false;

  uint64_t type = static_cast<uint64_t>(ObserverType::CPU) |
                  static_cast<uint64_t>(ObserverType::INTERVAL) |
                  static_cast<uint64_t>(ObserverType::CPU_INSTRUCTIONS);

  /* Defines the commands and paths required */
  std::filesystem::path tmp_folder = this->record_.tmp_folder_path_;
  this->annotation_ = tmp_folder / filename;
  this->command_prefix_ = std::string("cd ") + std::string(tmp_folder);
  this->command_prefix_ += " && perf annotate --percent-type global-period -i ";
  this->command_suffix_ = " | sort -r -k2,1n";
  this->command_suffix_ += " > " + filename;

  this->caps_.emplace_back();
  this->caps_[0].type = type;

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || \
    defined(__i386__) || defined(__i386) || defined(_M_IX86)
  this->classifier_ = AsmClassifier::Build(assembly::Architecture::X86);
#else
  this->classifier_ = nullptr;
#endif
}

Status PerfAnnotateObserver::Trigger() {
  Status ret{};

  if (!this->record_.valid_)
    return Status{Status::NOT_READY, "Not ready to query"};

  /* Executing the annotate command */
  std::string cmd = this->command_prefix_ +
                    std::string(this->record_.path_to_perf_data_) +
                    this->command_suffix_;
  int retv = std::system(cmd.c_str());
  if (retv) {
    ret = Status{Status::FILE_ERROR, "Cannot execute perf annotate command"};
  }

  /* Parsing the results */
  ret = this->ParseResults();
  return ret;
}

Status PerfAnnotateObserver::ParseResults() {
  std::ifstream ann_file(this->annotation_);
  if (!ann_file.is_open()) {
    this->valid_ = false;
    return Status{Status::FILE_ERROR, "Cannot open annotation file"};
  }

  /* Cleat the histogram */
  this->readings_.histogram.clear();

  /* Read the file line by line */
  std::string line;
  while (std::getline(ann_file, line)) {
    /* Variables of interest */
    float percent = 0.f;
    std::string assembly;

    /* Intermediate */
    std::stringstream sloc;
    std::string drop;

    sloc << line;
    sloc >> percent;

    /* Continue if cannot parse */
    if (!sloc.good()) continue;

    /* Parse */
    if (PERF_ANNOTATE_THRES >= percent) continue;
    sloc >> drop; /* Get rid of ':' */
    sloc >> drop; /* Get rid of 'address' */
    sloc >> assembly;

    /* Add to the histogram */
    if (this->readings_.histogram.find(assembly) ==
        this->readings_.histogram.end()) {
      this->readings_.histogram[assembly] = percent;
    } else {
      this->readings_.histogram[assembly] += percent;
    }

    /* Classify */
    if (!this->classifier_) continue;
    InstructionPair classification = this->classifier_->Classify(assembly);
    if (this->readings_.classification[classification.first].find(
            classification.second) ==
        this->readings_.classification[classification.first].end()) {
      this->readings_
          .classification[classification.first][classification.second] =
          percent;
    } else {
      this->readings_
          .classification[classification.first][classification.second] +=
          percent;
    }
  }

  this->valid_ = true;
  return Status{};
}

std::vector<Readings*> PerfAnnotateObserver::GetReadings() {
  return std::vector<Readings*>{static_cast<Readings*>(&(this->readings_))};
}

Status PerfAnnotateObserver::SelectDevice(const uint /* device */) {
  return Status{
      Status::NOT_IMPLEMENTED,
      "It is not possible to select a device since this is a wrapper class"};
}

Status PerfAnnotateObserver::SetScope(const ObserverScope /* scope */) {
  return Status{
      Status::NOT_IMPLEMENTED,
      "It is not possible change the scope since this is a wrapper class"};
}

Status PerfAnnotateObserver::SetPID(const uint /* pid */) {
  return Status{
      Status::NOT_IMPLEMENTED,
      "It is not possible change the PID since this is a wrapper class"};
}

ObserverScope PerfAnnotateObserver::GetScope() const noexcept {
  return this->record_.GetScope();
}

uint PerfAnnotateObserver::GetPID() const noexcept {
  return this->record_.GetPID();
}

const std::vector<ObserverCapabilities>& PerfAnnotateObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status PerfAnnotateObserver::GetStatus() { return Status{}; }

Status PerfAnnotateObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status PerfAnnotateObserver::ClearInterval() { return Status{}; }

Status PerfAnnotateObserver::Reset() {
  this->readings_.timestamp = 0;
  this->readings_.difference = 0;
  this->valid_ = false;
  this->readings_.type = static_cast<int>(ObserverType::CPU);
  return Status{};
}

PerfAnnotateObserver::~PerfAnnotateObserver() {}

} /* namespace efimon */
