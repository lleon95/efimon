/**
 * @file annotate.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer to query the perf annotate command. This depends on the
 * perf record class
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/perf/annotate.hpp>
#include <efimon/readings.hpp>
#include <efimon/status.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <third-party/pstream.hpp>

#ifndef PERF_ANNOTATE_THRES
#define PERF_ANNOTATE_THRES 0.01
#endif /* PERF_ANNOTATE_THRES */

namespace efimon {

PerfAnnotateObserver::PerfAnnotateObserver(PerfRecordObserver& record)
    : Observer{}, record_{record} {
  this->valid_ = false;

  uint64_t type = static_cast<uint64_t>(ObserverType::CPU) |
                  static_cast<uint64_t>(ObserverType::INTERVAL) |
                  static_cast<uint64_t>(ObserverType::CPU_INSTRUCTIONS);

  /* Defines the commands and paths required */
  this->ReconstructPath();

  this->caps_.emplace_back();
  this->caps_[0].type = type;

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || \
    defined(__i386__) || defined(__i386) || defined(_M_IX86)
  this->classifier_ = AsmClassifier::Build(assembly::Architecture::X86);
#else
  this->classifier_ = nullptr;
#endif
}

void PerfAnnotateObserver::ReconstructPath() {
  std::filesystem::path tmp_folder = this->record_.tmp_folder_path_;
  this->command_prefix_ = std::string("cd ") + std::string(tmp_folder);
  this->command_prefix_ +=
      " && perf annotate -q --percent-type global-period -i ";
  this->command_suffix_ = " | sort -r -k2,1n";
}

Status PerfAnnotateObserver::Trigger() {
  Status ret{};

  if (!this->record_.valid_)
    return Status{Status::NOT_READY, "Not ready to query"};

  this->ReconstructPath();

  /* Executing the annotate command */
  std::string cmd = this->command_prefix_ +
                    std::string(this->record_.path_to_perf_data_) +
                    this->command_suffix_;

  redi::ipstream ip(cmd, redi::pstreambuf::pstdout);
  if (!ip.is_open()) {
    ret = Status{Status::FILE_ERROR, "Cannot execute perf annotate command"};
    return ret;
  }

  /* Parsing the results */
  ret = this->ParseResults(ip);
  return ret;
}

Status PerfAnnotateObserver::ParseResults(redi::ipstream& ip) {
  if (!ip.is_open()) {
    this->valid_ = false;
    return Status{Status::FILE_ERROR, "Cannot open annotation file"};
  }

  /* Cleat the histogram */
  this->readings_.histogram.clear();
  this->readings_.classification.clear();

  /* Read the file line by line */
  std::string line;
  while (std::getline(ip, line)) {
    /* Variables of interest */
    float percent = 0.f;
    std::string assembly;

    /* Intermediate */
    std::stringstream sloc;
    std::string drop;
    std::string operands;

    sloc << line;
    sloc >> percent;

    /* Continue if cannot parse */
    if (!sloc.good()) continue;

    /* Parse */
    if (PERF_ANNOTATE_THRES >= percent) continue;
    sloc >> drop; /* Get rid of ':' */
    sloc >> drop; /* Get rid of 'address' */
    sloc >> assembly;
    sloc >> operands;

    /* Classify */
    if (!this->classifier_) continue;
    std::string optypes = this->classifier_->OperandTypes(operands);
    InstructionPair classification =
        this->classifier_->Classify(assembly, optypes);
    assembly += std::string("_") + optypes;

    /* Add to the histogram */
    if (this->readings_.histogram.find(assembly) ==
        this->readings_.histogram.end()) {
      this->readings_.histogram[assembly] = percent;
    } else {
      this->readings_.histogram[assembly] += percent;
    }

    /* Handle the creation of the maps */
    bool family_found =
        this->readings_.classification[std::get<0>(classification)].find(
            std::get<1>(classification)) !=
        this->readings_.classification[std::get<0>(classification)].end();
    if (!family_found) {
      this->readings_.classification[std::get<0>(classification)]
                                    [std::get<1>(classification)] = {};
    }
    bool origin_found = this->readings_
                            .classification[std::get<0>(classification)]
                                           [std::get<1>(classification)]
                            .find(std::get<2>(classification)) !=
                        this->readings_
                            .classification[std::get<0>(classification)]
                                           [std::get<1>(classification)]
                            .end();
    if (!origin_found) {
      this->readings_.classification[std::get<0>(classification)][std::get<1>(
          classification)][std::get<2>(classification)] = 0.f;
    }

    this->readings_.classification[std::get<0>(classification)][std::get<1>(
        classification)][std::get<2>(classification)] += percent;
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
