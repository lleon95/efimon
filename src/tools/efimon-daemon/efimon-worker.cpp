/**
 * @file efimon-worker.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Defines the process daemon attachable worker
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include "efimon-daemon/efimon-worker.hpp"  // NOLINT

#include <array>
#include <efimon/logger/csv.hpp>
#include <efimon/logger/macros.hpp>
#include <efimon/perf/annotate.hpp>
#include <efimon/perf/record.hpp>
#include <efimon/proc/process-tree.hpp>
#include <efimon/proc/stat.hpp>
#include <efimon/proc/thread-tree.hpp>
#include <efimon/ptrace-capstone/ptrace-capstone.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "efimon-daemon/efimon-analyser.hpp"  // NOLINT
#include "macro-handling.hpp"                 // NOLINT

namespace efimon {

EfimonWorker::EfimonWorker()
    : name_{},                 // NOLINT
      pid_{0},                 // NOLINT
      samples_{0},             // NOLINT
      running_{false},         // NOLINT
      analyser_{nullptr},      // NOLINT
      thread_{nullptr},        // NOLINT
      tree_{nullptr},          // NOLINT
      cpids_{},                // NOLINT
      proc_meter_{},           // NOLINT
      perf_record_meter_{},    // NOLINT
      perf_annotate_meter_{},  // NOLINT
      ptrace_meter_{} {}       // NOLINT

EfimonWorker::EfimonWorker(const std::string &name, const uint pid,
                           EfimonAnalyser *analyser)  // NOLINT
    : name_{name},                                    // NOLINT
      pid_{pid},                                      // NOLINT
      samples_{0},                                    // NOLINT
      running_{false},                                // NOLINT
      analyser_{analyser},                            // NOLINT
      thread_{nullptr},                               // NOLINT
      tree_{nullptr},                                 // NOLINT
      cpids_{},                                       // NOLINT
      proc_meter_{},                                  // NOLINT
      perf_record_meter_{},                           // NOLINT
      perf_annotate_meter_{},                         // NOLINT
      ptrace_meter_{} {}                              // NOLINT

EfimonWorker::EfimonWorker(EfimonWorker &&worker)
    : name_{std::move(worker.name_)},                                // NOLINT
      pid_{std::move(worker.pid_)},                                  // NOLINT
      samples_{std::move(worker.samples_)},                          // NOLINT
      running_{false},                                               // NOLINT
      analyser_{std::move(worker.analyser_)},                        // NOLINT
      thread_{nullptr},                                              // NOLINT
      tree_{nullptr},                                                // NOLINT
      cpids_{},                                                      // NOLINT
      proc_meter_{std::move(worker.proc_meter_)},                    // NOLINT
      perf_record_meter_{std::move(worker.perf_record_meter_)},      // NOLINT
      perf_annotate_meter_{std::move(worker.perf_annotate_meter_)},  // NOLINT
      ptrace_meter_{std::move(worker.ptrace_meter_)} {               // NOLINT
  this->running_.store(worker.running_.load());
  this->thread_.swap(worker.thread_);
}

EfimonWorker::~EfimonWorker() { this->Stop(); }

Status EfimonWorker::Start(const uint delay, const uint samples,
                           const uint perf, const uint freq,  // NOLINT
                           const uint delay_perf,             // NOLINT
                           const int children,                // NOLINT
                           const int cthreads) {              // NOLINT
  if (0 == this->pid_) {
    EFM_ERROR_STATUS(
        "Invalid instance of the worker. Are you using default constructor?",
        Status::CANNOT_OPEN);
  }

  // Get the process tree looking for children
  this->tree_ = std::make_unique<ProcessTree>(this->pid_);
  EFM_CHECK_STATUS(this->tree_->Refresh());
  auto children_pids = this->tree_->GetTree();
  int total_children = static_cast<int>(children_pids.size()) - 1;
  int analysis_children = children == -1 ? total_children : children;

  EFM_INFO("Process Monitor Start for PID: " + std::to_string(this->pid_) +
           " with delay: " + std::to_string(delay) + " and samples: " +
           std::to_string(samples) + " and perf " + std::to_string(perf) +
           " at: " + std::to_string(freq) + " and children under analysis: " +
           std::to_string(analysis_children) + "/" +
           std::to_string(total_children) +
           " with time window: " + std::to_string(delay_perf) + " secs");
  this->samples_ = samples;

  // Create observers for each PID
  EFM_INFO("Starting the monitoring of children processes of parent PID: " +
           std::to_string(this->pid_));
  for (const int pid : children_pids) {
    this->cpids_.push_back(pid);

    // Get the threads
    this->pid_threads_[pid] = std::vector<int>{};
    EFM_INFO("Threads IDs from PID " + std::to_string(pid));
    ThreadTree thread_tree{pid};

    auto tree_vector = thread_tree.GetTree();
    int total_threads = tree_vector.size();
    int analysis_threads = cthreads == -1 ? total_threads : cthreads;
    analysis_threads = analysis_threads == 0 ? 1 : analysis_threads;
    analysis_threads =
        analysis_threads > total_threads ? total_threads : analysis_threads;

    for (int tid : tree_vector) {
      EFM_INFO("\t" + std::to_string(tid));
      this->pid_threads_[pid].push_back(tid);
    }

    EFM_INFO("Analysing only " + std::to_string(analysis_threads) + "/" +
             std::to_string(total_threads));
    // TODO(lleon): Add the tasks option
    // Create meters for children PID
    this->proc_meter_[pid] = CreateIfEnabled<ProcStatObserver, true>(
        pid, efimon::ObserverScope::PROCESS, delay);
    if (ASM_WITH_PERF == perf) {
#ifdef ENABLE_PERF
      EFM_INFO("Process Monitor Start using Linux Perf to PID " +
               std::to_string(pid));
      auto perf_record_meter_iface = std::make_shared<PerfRecordObserver>(
          pid, efimon::ObserverScope::PROCESS, delay_perf, freq, true);
      this->perf_record_meter_[pid] = perf_record_meter_iface;
      this->perf_annotate_meter_[pid] =
          std::make_shared<PerfAnnotateObserver>(*perf_record_meter_iface);
#else
      EFM_INFO("Process Monitor did not start using Linux Perf to PID " +
               std::to_string(pid));
      this->perf_record_meter_[pid] = nullptr;
      this->perf_annotate_meter_[pid] = nullptr;
#endif
    } else if (ASM_WITH_PTRACE == perf) {
#ifdef ENABLE_PTRACE_CAPSTONE
      EFM_INFO("Process Monitor Start using PTrace-Capstone to PID " +
               std::to_string(pid));
      this->ptrace_meter_[pid] = std::make_shared<PTraceCapstoneObserver>(
          pid, efimon::ObserverScope::PROCESS, delay_perf * 1000, freq);
#else
      EFM_INFO("Process Monitor did not start using PTrace-Capstone to PID " +
               std::to_string(pid));
      this->ptrace_meter_[pid] = nullptr;
#endif
    }
    if (--analysis_children < 0) {
      break;
    }
  }

  this->thread_ = std::make_unique<std::thread>(&EfimonWorker::ProcStatsWorker,
                                                this, delay, this->cpids_);

  return Status{};
}
Status EfimonWorker::Stop() {
  if (0 == this->pid_) {
    EFM_ERROR_STATUS(
        "Invalid instance of the worker. Are you using default constructor?",
        Status::CANNOT_OPEN);
  }

  this->running_.store(false);
  if (this->thread_) {
    this->thread_->join();
    this->thread_.reset();
    EFM_INFO("Process Monitor Stopped for PID: " + std::to_string(this->pid_));
  }

  // Destroy observers
  this->proc_meter_.clear();
  this->perf_record_meter_.clear();
  this->perf_annotate_meter_.clear();
  this->ptrace_meter_.clear();
  this->tree_.reset();
  this->cpu_usage_.clear();
  this->instructions_samples_.clear();

  return Status{};
}

Status EfimonWorker::State() {
  auto code = this->running_.load() ? Status::RUNNING : Status::STOPPED;
  return Status{code, std::to_string(static_cast<uint>(code))};
}

void EfimonWorker::ProcStatsWorker(const uint delay,
                                   const std::vector<int> &pids) {  // NOLINT
  bool first_sample = true;
  bool enabled_perf = false;
  bool enabled_samples = false;
  this->running_.store(true);

  this->mutex_.lock();
  this->log_table_.clear();
  for (const int pid : pids) {
    this->cpu_usage_[pid] =
        GetReadingsIfEnabled<CPUReadings, true>(this->proc_meter_[pid], 0);

    enabled_perf = this->perf_annotate_meter_[pid] != nullptr ||
                   this->ptrace_meter_[pid] != nullptr;

    if (this->perf_annotate_meter_[pid] != nullptr) {
      this->instructions_samples_[pid] =
          GetReadingsIfEnabled<InstructionReadings, true>(
              this->perf_annotate_meter_[pid], 0);
    } else if (this->ptrace_meter_[pid] != nullptr) {
      this->instructions_samples_[pid] =
          GetReadingsIfEnabled<InstructionReadings, true>(
              this->ptrace_meter_[pid], 0);
    }
  }
  enabled_samples = this->samples_ != 0;
  this->mutex_.unlock();

  EFM_CHECK(this->CreateLogTable(), EFM_WARN);
  std::unordered_map<int, std::shared_ptr<CSVLogger>> loggers;

  for (const int pid : pids) {
    std::string name = this->name_ + "." + std::to_string(pid);
    EFM_INFO("Process with PID " + std::to_string(pid) +
             " will be recorded in: " + name);
    loggers[pid] = std::make_shared<CSVLogger>(name, this->log_table_);
  }

  while (running_.load()) {
    for (const int pid : pids) {
      EFM_CHECK(RefreshProcStat(pid), EFM_WARN_AND_BREAK);

      // Log results
      if (!first_sample) {
        std::shared_ptr<CSVLogger> logger = loggers[pid];
        if (logger) {
          EFM_CHECK(LogReadings(*logger, pid), EFM_WARN_AND_BREAK);
        } else {
          EFM_WARN("Cannot get the logger for the PID: " + std::to_string(pid));
        }
      }
    }

    first_sample = false;

    // Wait for the next sample. Perf is a blocking call
    if (!enabled_perf) {
      std::this_thread::sleep_for(std::chrono::seconds(delay));
    }

    // Check if enabled samples
    if (enabled_samples && --(this->samples_) == 0) {
      running_.store(false);
    }
    EFM_DEBUG(analyser_->IsDebugged(),
              "Process with PID " + std::to_string(this->pid_) + " updated");
  }

  EFM_INFO("Monitoring of PID " + std::to_string(this->pid_) + " ended");
}

Status EfimonWorker::RefreshProcStat(const int pid) {
  std::scoped_lock slock(this->mutex_);
  EFM_CHECK_STATUS(TriggerIfEnabled(this->proc_meter_[pid]));
  EFM_CHECK_STATUS(TriggerIfEnabled(this->perf_record_meter_[pid]));
  EFM_CHECK_STATUS(TriggerIfEnabled(this->perf_annotate_meter_[pid]));
  EFM_CHECK_STATUS(TriggerIfEnabled(this->ptrace_meter_[pid]));
  return Status{};
}

Status EfimonWorker::CreateLogTable() {
  std::scoped_lock slock(this->mutex_);
  // Timestamping
  this->log_table_.push_back({"Timestamp", Logger::FieldType::INTEGER64});
  this->log_table_.push_back({"TimeDifference", Logger::FieldType::INTEGER64});
  // System and Process CPU usage
  this->log_table_.push_back({"SystemCpuUsage", Logger::FieldType::FLOAT});
  this->log_table_.push_back({"ProcessCpuUsage", Logger::FieldType::FLOAT});
  // Socket frequencies
  CPUReadings sys_cpu_readings{};
  EFM_CHECK(this->analyser_->GetReadings(3, sys_cpu_readings), EFM_WARN);
  for (uint i = 0; i < sys_cpu_readings.socket_frequency.size(); ++i) {
    std::string name = "SocketFreq";
    name += std::to_string(i);
    this->log_table_.push_back({name, Logger::FieldType::FLOAT});
  }

  // Add the IPMI values
#ifdef ENABLE_IPMI
  PSUReadings psu_readings;
  FanReadings fan_readings;
  this->analyser_->GetReadings(0, psu_readings);
  this->analyser_->GetReadings(1, fan_readings);
  uint psu_num = psu_readings.psu_max_power.size();
  uint fan_num = fan_readings.fan_speeds.size();
  for (uint i = 0; i < psu_num; ++i) {
    std::string name = "PSUPower";
    name += std::to_string(i);
    this->log_table_.push_back({name, Logger::FieldType::FLOAT});
  }
  for (uint i = 0; i < fan_num; ++i) {
    std::string name = "FanSpeed";
    name += std::to_string(i);
    this->log_table_.push_back({name, Logger::FieldType::FLOAT});
  }
#endif
  // Add the RAPL values
#ifdef ENABLE_RAPL
  CPUReadings rapl_readings;
  this->analyser_->GetReadings(2, rapl_readings);
  uint socket_num = rapl_readings.socket_power.size();
  for (uint i = 0; i < socket_num; ++i) {
    std::string name = "SocketPower";
    name += std::to_string(i);
    this->log_table_.push_back({name, Logger::FieldType::FLOAT});
  }
#endif

#if defined(ENABLE_PERF) || defined(ENABLE_PTRACE_CAPSTONE)
  if ((0 != this->perf_record_meter_.size()) ||
      0 != this->ptrace_meter_.size()) {
    for (uint itype = 0;
         itype <= static_cast<uint>(assembly::InstructionType::UNCLASSIFIED);
         ++itype) {
      auto type = static_cast<assembly::InstructionType>(itype);
      std::string stype = AsmClassifier::TypeString(type);
      for (uint ftype = 0;
           ftype < static_cast<uint>(assembly::InstructionFamily::OTHER);
           ++ftype) {
        auto family = static_cast<assembly::InstructionFamily>(ftype);
        std::string sfamily = AsmClassifier::FamilyString(family);
        if (family == assembly::InstructionFamily::MEMORY ||
            family == assembly::InstructionFamily::ARITHMETIC ||
            family == assembly::InstructionFamily::LOGIC) {
          this->log_table_.push_back(
              {std::string("ProbabilityRegister") + stype + sfamily,
               Logger::FieldType::FLOAT});
          this->log_table_.push_back(
              {std::string("ProbabilityMemLoad") + stype + sfamily,
               Logger::FieldType::FLOAT});
          this->log_table_.push_back(
              {std::string("ProbabilityMemStore") + stype + sfamily,
               Logger::FieldType::FLOAT});
          this->log_table_.push_back(
              {std::string("ProbabilityMemUpdate") + stype + sfamily,
               Logger::FieldType::FLOAT});
        } else {
          std::string name = "Probability";
          name += stype + sfamily;
          this->log_table_.push_back({name, Logger::FieldType::FLOAT});
        }
      }
    }
  }
#endif

  return Status{};
}
Status EfimonWorker::LogReadings(CSVLogger &logger, const int pid) {  // NOLINT
  std::scoped_lock slock(this->mutex_);

  if (!this->cpu_usage_[pid]) {
    return Status{Status::NOT_FOUND, "Cannot find the CPU Usage"};
  }

  CPUReadings sys_cpu_readings{};
  EFM_CHECK(this->analyser_->GetReadings(3, sys_cpu_readings), EFM_WARN);

  auto timestamp = this->cpu_usage_[pid]->timestamp;
  auto difference = this->cpu_usage_[pid]->difference;
  auto proc_usage = this->cpu_usage_[pid]->overall_usage;
  auto sys_usage = sys_cpu_readings.overall_usage;

  std::unordered_map<std::string, std::shared_ptr<Logger::IValue>> values = {};

  LOG_VAL(values, "Timestamp", timestamp);
  LOG_VAL(values, "SystemCpuUsage", sys_usage);
  LOG_VAL(values, "ProcessCpuUsage", proc_usage);
  LOG_VAL(values, "TimeDifference", difference);
  for (uint i = 0; i < sys_cpu_readings.socket_frequency.size(); ++i) {
    std::string name = "SocketFreq";
    name += std::to_string(i);
    LOG_VAL(values, name, sys_cpu_readings.socket_frequency[i]);
  }

#ifdef ENABLE_IPMI
  PSUReadings psu_readings{};
  FanReadings fan_readings{};

  EFM_CHECK(this->analyser_->GetReadings(0, psu_readings), EFM_WARN);
  EFM_CHECK(this->analyser_->GetReadings(1, fan_readings), EFM_WARN);
  uint psu_num = psu_readings.psu_max_power.size();
  uint fan_num = fan_readings.fan_speeds.size();

  for (uint i = 0; i < psu_num; ++i) {
    std::string name = "PSUPower";
    name += std::to_string(i);
    LOG_VAL(values, name, psu_readings.psu_power.at(i));
  }
  for (uint i = 0; i < fan_num; ++i) {
    std::string name = "FanSpeed";
    name += std::to_string(i);
    LOG_VAL(values, name, fan_readings.fan_speeds.at(i));
  }
#endif

#ifdef ENABLE_RAPL
  CPUReadings rapl_readings{};
  EFM_CHECK(this->analyser_->GetReadings(2, rapl_readings), EFM_WARN);
  uint socket_num = rapl_readings.socket_power.size();
  for (uint i = 0; i < socket_num; ++i) {
    std::string name = "SocketPower";
    name += std::to_string(i);
    LOG_VAL(values, name, rapl_readings.socket_power.at(i));
  }
#endif

#if defined(ENABLE_PERF) || defined(ENABLE_PTRACE_CAPSTONE)
  if ((this->perf_record_meter_.begin()->second &&
       this->perf_annotate_meter_.begin()->second) ||
      this->ptrace_meter_.begin()->second) {
    for (uint itype = 0;
         itype <= static_cast<uint>(assembly::InstructionType::UNCLASSIFIED);
         ++itype) {
      for (uint ftype = 0;
           ftype < static_cast<uint>(assembly::InstructionFamily::OTHER);
           ++ftype) {
        auto type = static_cast<assembly::InstructionType>(itype);
        std::string stype = AsmClassifier::TypeString(type);
        auto family = static_cast<assembly::InstructionFamily>(ftype);
        std::string sfamily = AsmClassifier::FamilyString(family);
        auto tit = instructions_samples_[pid]->classification.find(type);

        if (family == assembly::InstructionFamily::MEMORY ||
            family == assembly::InstructionFamily::ARITHMETIC ||
            family == assembly::InstructionFamily::LOGIC) {
          std::array<float, 4> probs;
          probs.fill(0.f);

          if (instructions_samples_[pid]->classification.end() != tit) {
            auto fit = tit->second.find(family);
            if (tit->second.end() != fit) {
              for (auto origit = fit->second.begin();
                   origit != fit->second.end(); origit++) {
                auto pairorigin =
                    AsmClassifier::OriginDecomposed(origit->first);
                if (pairorigin.first == assembly::DataOrigin::MEMORY &&
                    pairorigin.second == assembly::DataOrigin::MEMORY) {
                  std::string fieldname = "ProbabilityMemUpdate";
                  LOG_VAL(values, fieldname + stype + sfamily, origit->second);
                } else if (pairorigin.first == assembly::DataOrigin::MEMORY) {
                  std::string fieldname = "ProbabilityMemLoad";
                  LOG_VAL(values, fieldname + stype + sfamily, origit->second);
                } else if (pairorigin.second == assembly::DataOrigin::MEMORY) {
                  std::string fieldname = "ProbabilityMemStore";
                  LOG_VAL(values, fieldname + stype + sfamily, origit->second);
                } else {
                  std::string fieldname = "ProbabilityRegister";
                  LOG_VAL(values, fieldname + stype + sfamily, origit->second);
                }
              }
            }
          }
        } else {
          float probres = 0.f;
          if (instructions_samples_[pid]->classification.end() != tit) {
            auto fit = tit->second.find(family);
            if (tit->second.end() != fit) {
              for (auto origit = fit->second.begin();
                   origit != fit->second.end(); origit++) {
                probres += origit->second;
              }
            }
          }
          std::string name = "Probability";
          name += stype + sfamily;
          LOG_VAL(values, name, probres);
        }
      }
    }
  }
#endif
  return logger.InsertRow(values);
}

}  // namespace efimon
