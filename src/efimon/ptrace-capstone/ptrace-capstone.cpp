/**
 * @file ptrace-capstone.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Ptrace-capstone-based ASM low-weight extractor
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <capstone/capstone.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/ptrace-capstone/ptrace-capstone.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/instruction-readings.hpp>
#include <efimon/status.hpp>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define CHECK_OR_RETURN(inst)               \
  {                                         \
    auto ret = (inst);                      \
    if (ret.code != Status::OK) return ret; \
  }

namespace efimon {

extern uint64_t GetUptime();

PTraceCapstoneObserver::PTraceCapstoneObserver(const uint pid,
                                               const ObserverScope scope,
                                               const uint64_t interval,
                                               const uint64_t frequency)
    : Observer{}, readings_{}, frequency_{frequency}, valid_{false} {
  this->pid_ = pid;
  this->interval_ = interval;
  this->worker_running_.store(false);
  this->worker_thread_ = nullptr;
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

  this->Reset();
}

Status PTraceCapstoneObserver::GetSample() {
  Status ret{};

  /* Attach the ptrace to the current process */
  if (ptrace(PTRACE_ATTACH, this->pid_, nullptr, nullptr) == -1) {
    ret = Status{Status::ACCESS_DENIED, "Cannot attach the ptrace to the PID"};
    return ret;
  }

  /* Watit the PID to change state */
  waitpid(this->pid_, nullptr, 0);

  /* Get the registers for the external PID */
  struct user_regs_struct regs;
  if (ptrace(PTRACE_GETREGS, this->pid_, nullptr, &regs) == -1) {
    ret = Status{Status::CANNOT_OPEN, "Cannot get the registers for the PID"};
    ptrace(PTRACE_DETACH, this->pid_, nullptr, nullptr);
    return ret;
  }

  /* Detach after retrieving PC */
  if (ptrace(PTRACE_DETACH, this->pid_, nullptr, nullptr) == -1) {
    ret = Status{Status::CONFIGURATION_ERROR,
                 "Cannot attach the ptrace to the PID"};
    return ret;
  }

  /* TODO(lleon): Add support for other architectures */
#if defined(__x86_64__)
  this->pc_ = regs.rip;
#else
#error "Unsupported architecture"
#endif

  /* Read the memory */
  char filename[64];
  snprintf(filename, sizeof(filename), "/proc/%d/mem", this->pid_);
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    ret = Status{Status::CANNOT_OPEN,
                 "The memory from the target proccess cannot be opened"};
    return ret;
  }
  ssize_t size = sizeof(this->pc_inst_mem_);
  if (pread(fd, this->pc_inst_mem_, size, this->pc_) != size) {
    ret = Status{Status::CANNOT_OPEN,
                 "The memory from the target proccess cannot be read"};
    close(fd);
    return ret;
  }

  close(fd);
  return ret;
}

Status PTraceCapstoneObserver::DecodeSample() {
  Status ret{};

  csh handle = 0;
  cs_insn *insn = nullptr;
  size_t count = 0;

  if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
    ret = Status{Status::CONFIGURATION_ERROR, "Cannot initialise Capstone"};
    return ret;
  }

  /* Switch to AT&T for the syntax */
  cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);

  count = cs_disasm(handle, this->pc_inst_mem_, sizeof(this->pc_inst_mem_),
                    this->pc_, 1, &insn);

  if (count <= 0) {
    ret = Status{Status::CANNOT_OPEN, "Cannot decode the instruction"};
  } else {
    this->inst_ =
        std::string(insn[0].mnemonic) + " " + std::string(insn[0].op_str);
    cs_free(insn, count);
  }

  cs_close(&handle);
  return ret;
}

Status PTraceCapstoneObserver::ParseResults() {
  /* Read the file line by line */
  std::string line;

  /* Intermediate */
  std::stringstream sloc;
  std::string drop;
  std::string operands;

  /* Variables of interest */
  this->samples_++;
  std::string assembly;

  sloc << this->inst_;
  sloc >> assembly;
  operands = sloc.str();

  /* Classify */
  if (!this->classifier_)
    return Status{Status::MEMBER_ABSENT, "Cannot get classifier"};
  std::string optypes = this->classifier_->OperandTypes(operands);
  InstructionPair classification =
      this->classifier_->Classify(assembly, optypes);
  assembly += std::string("_") + optypes;

  /* Add to the histogram */
  if (this->readings_.histogram.find(assembly) ==
      this->readings_.histogram.end()) {
    this->readings_.histogram[assembly] = 0;
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
      classification)][std::get<2>(classification)] += 100.f;
  this->readings_.histogram[assembly] += 100.f;

  this->valid_ = true;
  return Status{};
}

Status PTraceCapstoneObserver::NormaliseResults() {
  for (auto &pair : this->readings_.histogram) {
    pair.second /= this->samples_;
  }

  for (auto &type : this->readings_.classification) {
    for (auto &family : type.second) {
      for (auto &origin : family.second) {
        origin.second /= this->samples_;
      }
    }
  }

  return Status{};
}

Status PTraceCapstoneObserver::Trigger() {
  Status ret{};

  /* Clear the histogram */
  this->readings_.histogram.clear();
  this->readings_.classification.clear();
  this->samples_ = 0;

  /* Launch the worker */
  this->worker_running_.store(true);
  this->worker_thread_ =
      std::make_unique<std::thread>(&PTraceCapstoneObserver::Worker, this);

  /* Wait until completion */
  std::this_thread::sleep_for(std::chrono::milliseconds(this->interval_));
  this->worker_running_.store(false);
  {
    std::unique_lock lk(this->worker_mutex_);
    this->worker_cv_.wait_for(lk, std::chrono::milliseconds(1));
  }

  this->worker_thread_->join();
  this->worker_thread_.reset(nullptr);
  this->NormaliseResults();
  return ret;
}

void PTraceCapstoneObserver::Worker() {
  uint64_t delay_sample_us = static_cast<uint64_t>(1e6 / this->frequency_);
  while (this->worker_running_.load()) {
    std::scoped_lock lock(this->worker_mutex_);

    /* Get the result */
    this->GetSample();
    this->DecodeSample();
    this->ParseResults();

    std::this_thread::sleep_for(std::chrono::microseconds(delay_sample_us));
  }
}

std::vector<Readings *> PTraceCapstoneObserver::GetReadings() {
  std::scoped_lock lock(this->worker_mutex_);
  return std::vector<Readings *>{static_cast<Readings *>(&(this->readings_))};
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

const std::vector<ObserverCapabilities>
    &PTraceCapstoneObserver::GetCapabilities() const noexcept {
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

  /* Clear the histogram */
  this->readings_.histogram.clear();
  this->readings_.classification.clear();
  this->samples_ = 0;
  return Status{};
}

PTraceCapstoneObserver::~PTraceCapstoneObserver() {}

} /* namespace efimon */
