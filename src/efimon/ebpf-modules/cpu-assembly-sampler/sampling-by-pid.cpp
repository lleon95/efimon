/**
 * @file sampling-by-pid.cpp
 * @author Diego Avila (diego.avila@uned.cr)
 * @brief Sampling-by-PID eBPF-based CPU cycle sampler observer
 *
 * @copyright Copyright (c) 2026. See License for Licensing
 */

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <capstone/capstone.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <efimon/asm-classifier.hpp>
#include <efimon/ebpf-modules/cpu-assembly-sampler/sampling-by-pid.hpp>
#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/instruction-readings.hpp>
#include <efimon/status.hpp>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include "prog.skel.h"  // NOLINT
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace efimon {

/**
 * @brief eBPF sample structure matching the kernel-side sample_t
 */
struct BPFSample {
  uint32_t pid;
  uint32_t tid;
  uint64_t ip;
  uint64_t ts;
};

/**
 * @brief Callback context for the ring buffer
 */
struct RBContext {
  SamplingByPIDObserver *observer;
};

static int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                           int group_fd, uint64_t flags) {
  return static_cast<int>(
      syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags));
}

/**
 * @brief Ring buffer callback: just collect the raw sample into a vector
 */
int handle_rb_event(void *ctx, void *data, size_t size) {
  if (size != sizeof(BPFSample) || !ctx || !data) return 0;
  auto *rb_ctx = static_cast<RBContext *>(ctx);
  auto *s = static_cast<BPFSample *>(data);
  rb_ctx->observer->collected_samples_.push_back(
      {s->pid, s->tid, s->ip, s->ts});
  return 0;
}

SamplingByPIDObserver::SamplingByPIDObserver(const uint pid,
                                             const ObserverScope scope,
                                             const uint64_t interval,
                                             const uint64_t frequency)
    : Observer{},
      readings_{},
      frequency_{frequency},
      valid_{false},
      ring_buffer_fd_{-1},
      bpf_skeleton_{nullptr},
      current_ip_{0},
      current_sample_tid_{0},
      current_sample_ts_{0},
      inst_mem_{},
      samples_collected_{0} {
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

Status SamplingByPIDObserver::InitializeBPF() {
  /* Open and load the eBPF skeleton */
  auto *skel = prog_bpf__open_and_load();
  if (!skel) {
    return Status{Status::CONFIGURATION_ERROR,
                  "Failed to open/load BPF skeleton"};
  }
  this->bpf_skeleton_ = static_cast<void *>(skel);

  int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
  if (ncpus <= 0) ncpus = 1;

  int prog_fd = bpf_program__fd(skel->progs.on_sample);
  bool attached = false;
  int last_errno = 0;

  this->perf_fds_.resize(ncpus, -1);

  /* Cap frequency to the kernel max to avoid EINVAL */
  uint64_t freq = this->frequency_;
  std::ifstream max_rate_file("/proc/sys/kernel/perf_event_max_sample_rate");
  if (max_rate_file.is_open()) {
    uint64_t max_rate = 0;
    max_rate_file >> max_rate;
    if (max_rate > 0 && freq > max_rate) {
      freq = max_rate;
    }
  }

  /*
   * Try multiple perf event configurations with decreasing precision.
   * precise_ip=2 requires PEBS which may not be available on all hardware
   * or in virtual machines. Fall back to software CPU clock if hardware
   * counters are unavailable.
   */
  struct {
    uint32_t type;
    uint64_t config;
    uint32_t precise_ip;
  } attempts[] = {
      {PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, 2},
      {PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, 0},
      {PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK, 0},
  };

  for (const auto &attempt : attempts) {
    /* Reset state from previous attempt */
    for (int cpu = 0; cpu < ncpus; cpu++) {
      if (this->perf_fds_[cpu] >= 0) {
        close(this->perf_fds_[cpu]);
        this->perf_fds_[cpu] = -1;
      }
    }
    attached = false;

    struct perf_event_attr attr {};
    memset(&attr, 0, sizeof(attr));
    attr.type = attempt.type;
    attr.config = attempt.config;
    attr.size = sizeof(attr);
    attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME;
    attr.freq = 1;
    attr.sample_freq = freq;
    attr.precise_ip = attempt.precise_ip;
    attr.disabled = 0;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;

    for (int cpu = 0; cpu < ncpus; cpu++) {
      this->perf_fds_[cpu] =
          perf_event_open(&attr, static_cast<pid_t>(this->pid_), cpu, -1, 0);
      if (this->perf_fds_[cpu] < 0) {
        last_errno = errno;
        continue;
      }

      if (ioctl(this->perf_fds_[cpu], PERF_EVENT_IOC_SET_BPF, prog_fd) < 0) {
        last_errno = errno;
        close(this->perf_fds_[cpu]);
        this->perf_fds_[cpu] = -1;
        continue;
      }

      if (ioctl(this->perf_fds_[cpu], PERF_EVENT_IOC_ENABLE, 0) < 0) {
        last_errno = errno;
        close(this->perf_fds_[cpu]);
        this->perf_fds_[cpu] = -1;
        continue;
      }

      attached = true;
    }

    if (attached) break;
  }

  if (!attached) {
    std::string errmsg = "Failed to attach perf events for target PID: ";
    errmsg += strerror(last_errno);
    errmsg += " (errno=" + std::to_string(last_errno) + ")";
    this->CleanupBPF();
    return Status{Status::ACCESS_DENIED, errmsg};
  }

  /* Create the ring buffer to receive samples from the eBPF program */
  this->ring_buffer_fd_ = bpf_map__fd(skel->maps.rb);

  return Status{};
}

Status SamplingByPIDObserver::CleanupBPF() {
  /* Close perf event file descriptors */
  for (int fd : this->perf_fds_) {
    if (fd >= 0) close(fd);
  }
  this->perf_fds_.clear();

  /* Destroy the BPF skeleton */
  if (this->bpf_skeleton_) {
    prog_bpf__destroy(static_cast<prog_bpf *>(this->bpf_skeleton_));
    this->bpf_skeleton_ = nullptr;
  }

  this->ring_buffer_fd_ = -1;
  return Status{};
}

Status SamplingByPIDObserver::PollRingBuffer() {
  if (this->ring_buffer_fd_ < 0) {
    return Status{Status::NOT_READY, "Ring buffer not initialized"};
  }

  auto *skel = static_cast<prog_bpf *>(this->bpf_skeleton_);
  RBContext rb_ctx{this};
  struct ring_buffer *rb =
      ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_rb_event,
                       static_cast<void *>(&rb_ctx), nullptr);
  if (!rb) {
    return Status{Status::CONFIGURATION_ERROR,
                  "Failed to create ring buffer consumer"};
  }

  /* Poll until the worker is stopped */
  while (this->worker_running_.load()) {
    ring_buffer__poll(rb, 100);
  }

  /* Drain remaining events */
  ring_buffer__poll(rb, 0);

  ring_buffer__free(rb);
  return Status{};
}

Status SamplingByPIDObserver::DecodeInstruction(const uint64_t ip) {
  /* Read the memory from the process at the given IP */
  char filename[64];
  snprintf(filename, sizeof(filename), "/proc/%d/mem", this->pid_);
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    return Status{Status::CANNOT_OPEN,
                  "The memory from the target process cannot be opened"};
  }

  ssize_t size = sizeof(this->inst_mem_);
  if (pread(fd, this->inst_mem_, size, static_cast<off_t>(ip)) != size) {
    close(fd);
    return Status{Status::CANNOT_OPEN,
                  "The memory from the target process cannot be read"};
  }
  close(fd);

  /* Decode using Capstone */
  csh handle = 0;
  cs_insn *insn = nullptr;
  size_t count = 0;

  if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
    return Status{Status::CONFIGURATION_ERROR, "Cannot initialise Capstone"};
  }

  cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);

  count =
      cs_disasm(handle, this->inst_mem_, sizeof(this->inst_mem_), ip, 1, &insn);

  if (count <= 0) {
    cs_close(&handle);
    return Status{Status::CANNOT_OPEN, "Cannot decode the instruction"};
  }

  this->inst_ =
      std::string(insn[0].mnemonic) + " " + std::string(insn[0].op_str);
  cs_free(insn, count);
  cs_close(&handle);

  return Status{};
}

Status SamplingByPIDObserver::ProcessSample(const uint32_t /* pid */,
                                            const uint32_t /* tid */,
                                            const uint64_t ip,
                                            const uint64_t /* ts */) {
  this->current_ip_ = ip;

  /* Skip kernel addresses (x86_64 kernel space starts at 0xffff800000000000) */
  if (ip >= 0xffff800000000000ULL || ip == 0) {
    return Status{};
  }

  auto decode_ret = this->DecodeInstruction(ip);
  if (decode_ret.code != Status::OK) {
    /* Skip samples that cannot be decoded */
    return Status{};
  }

  this->ParseResults();
  this->samples_collected_++;
  return Status{};
}

Status SamplingByPIDObserver::ParseResults() {
  std::stringstream sloc;
  std::string assembly;
  std::string operands;

  sloc << this->inst_;
  sloc >> assembly;
  operands = sloc.str();

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

  /* Handle the creation of the classification maps */
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

Status SamplingByPIDObserver::NormalizeResults() {
  if (this->samples_collected_ == 0) return Status{};

  for (auto &pair : this->readings_.histogram) {
    pair.second /= this->samples_collected_;
  }

  for (auto &type : this->readings_.classification) {
    for (auto &family : type.second) {
      for (auto &origin : family.second) {
        origin.second /= this->samples_collected_;
      }
    }
  }

  return Status{};
}

Status SamplingByPIDObserver::Trigger() {
  Status ret{};

  /* Clear the histogram */
  this->readings_.histogram.clear();
  this->readings_.classification.clear();
  this->samples_collected_ = 0;
  this->collected_samples_.clear();

  /* Initialize eBPF program and perf events */
  ret = this->InitializeBPF();
  if (ret.code != Status::OK) return ret;

  /* Launch the worker to poll the ring buffer and collect raw samples */
  this->worker_running_.store(true);
  this->worker_thread_ =
      std::make_unique<std::thread>(&SamplingByPIDObserver::Worker, this);

  /* Wait for the specified interval */
  std::this_thread::sleep_for(std::chrono::milliseconds(this->interval_));

  /* Signal the worker to stop polling */
  this->worker_running_.store(false);

  {
    std::unique_lock lk(this->worker_mutex_);
    this->worker_cv_.wait_for(lk, std::chrono::seconds(5));
  }

  this->worker_thread_->join();
  this->worker_thread_.reset(nullptr);

  /* Clean up eBPF resources before processing (perf events no longer needed) */
  this->CleanupBPF();

  /* Now process the collected samples (decode + classify) */
  for (const auto &sample : this->collected_samples_) {
    this->ProcessSample(sample.pid, sample.tid, sample.ip, sample.ts);
  }

  this->NormalizeResults();
  return Status{};
}

void SamplingByPIDObserver::Worker() {
  /* Poll the eBPF ring buffer, collecting raw samples into collected_samples_
   */
  this->PollRingBuffer();
  this->worker_cv_.notify_one();
}

std::vector<Readings *> SamplingByPIDObserver::GetReadings() {
  std::scoped_lock lock(this->worker_mutex_);
  return std::vector<Readings *>{static_cast<Readings *>(&(this->readings_))};
}

Status SamplingByPIDObserver::SelectDevice(const uint /* device */) {
  return Status{
      Status::NOT_IMPLEMENTED,
      "It is not possible to select a device since this is a wrapper class"};
}

Status SamplingByPIDObserver::SetScope(const ObserverScope /* scope */) {
  return Status{
      Status::NOT_IMPLEMENTED,
      "It is not possible change the scope since this is a wrapper class"};
}

Status SamplingByPIDObserver::SetPID(const uint pid) {
  this->pid_ = pid;
  return Status{};
}

ObserverScope SamplingByPIDObserver::GetScope() const noexcept {
  return ObserverScope::PROCESS;
}

uint SamplingByPIDObserver::GetPID() const noexcept { return this->pid_; }

const std::vector<ObserverCapabilities>
    &SamplingByPIDObserver::GetCapabilities() const noexcept {
  return this->caps_;
}

Status SamplingByPIDObserver::GetStatus() { return Status{}; }

Status SamplingByPIDObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status SamplingByPIDObserver::ClearInterval() {
  this->interval_ = 0;
  return Status{Status::OK, "The clear interval has been cleared"};
}

Status SamplingByPIDObserver::Reset() {
  this->readings_.timestamp = 0;
  this->readings_.difference = 0;
  this->valid_ = false;
  this->readings_.type = static_cast<int>(ObserverType::CPU);

  /* Clear the histogram */
  this->readings_.histogram.clear();
  this->readings_.classification.clear();
  this->samples_collected_ = 0;
  return Status{};
}

SamplingByPIDObserver::~SamplingByPIDObserver() {
  if (this->worker_running_.load()) {
    this->worker_running_.store(false);
    if (this->worker_thread_ && this->worker_thread_->joinable()) {
      this->worker_thread_->join();
    }
  }
  this->CleanupBPF();
}

} /* namespace efimon */
