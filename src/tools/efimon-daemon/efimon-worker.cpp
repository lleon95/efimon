/**
 * @file efimon-worker.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Defines the process daemon attachable worker
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include "efimon-daemon/efimon-worker.hpp"  // NOLINT

#include <efimon/logger/csv.hpp>
#include <efimon/logger/macros.hpp>
#include <efimon/perf/annotate.hpp>
#include <efimon/perf/record.hpp>
#include <efimon/proc/stat.hpp>
#include <unordered_map>

#include "efimon-daemon/efimon-analyser.hpp"  // NOLINT
#include "macro-handling.hpp"                 // NOLINT
namespace efimon {

EfimonWorker::EfimonWorker()
    : name_{},
      pid_{0},
      samples_{0},
      running_{false},
      analyser_{nullptr},
      thread_{nullptr},
      proc_meter_{nullptr},
      perf_record_meter_{nullptr},
      perf_annotate_meter_{nullptr} {}

EfimonWorker::EfimonWorker(const std::string &name, const uint pid,
                           EfimonAnalyser *analyser)
    : name_{name},
      pid_{pid},
      samples_{0},
      running_{false},
      analyser_{analyser},
      thread_{nullptr},
      proc_meter_{nullptr},
      perf_record_meter_{nullptr},
      perf_annotate_meter_{nullptr} {}

EfimonWorker::EfimonWorker(EfimonWorker &&worker)
    : name_{std::move(worker.name_)},
      pid_{std::move(worker.pid_)},
      samples_{std::move(worker.samples_)},
      running_{false},
      analyser_{std::move(worker.analyser_)},
      thread_{nullptr},
      proc_meter_{std::move(worker.proc_meter_)},
      perf_record_meter_{std::move(worker.perf_record_meter_)},
      perf_annotate_meter_{std::move(worker.perf_annotate_meter_)} {
  this->running_.store(worker.running_.load());
  this->thread_.swap(worker.thread_);
}

EfimonWorker::~EfimonWorker() { this->Stop(); }

Status EfimonWorker::Start(const uint delay, const uint samples,
                           const bool enable_perf, const uint freq) {
  if (0 == this->pid_) {
    EFM_ERROR_STATUS(
        "Invalid instance of the worker. Are you using default constructor?",
        Status::CANNOT_OPEN);
  }
  EFM_INFO("Process Monitor Start for PID: " + std::to_string(this->pid_) +
           " with delay: " + std::to_string(delay) +
           " and samples: " + std::to_string(samples) + " and perf " +
           std::to_string(enable_perf) + " at: " + std::to_string(freq));
  this->samples_ = samples;
  // Create observers
  this->proc_meter_ = CreateIfEnabled<ProcStatObserver, true>(
      this->pid_, efimon::ObserverScope::PROCESS, delay);
  if (enable_perf) {
#ifdef ENABLE_PERF
    auto perf_record_meter_iface = std::make_shared<PerfRecordObserver>(
        this->pid_, efimon::ObserverScope::PROCESS, delay, freq, true);
    this->perf_record_meter_ = perf_record_meter_iface;
    this->perf_annotate_meter_ =
        std::make_shared<PerfAnnotateObserver>(*perf_record_meter_iface);
#else
    this->perf_record_meter_ = nullptr;
    this->perf_annotate_meter_ = nullptr;
#endif
  }

  this->thread_ = std::make_unique<std::thread>(&EfimonWorker::ProcStatsWorker,
                                                this, delay);

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
  this->proc_meter_.reset();
  this->perf_record_meter_.reset();
  this->perf_annotate_meter_.reset();
  this->cpu_usage_ = nullptr;
  this->instructions_samples_ = nullptr;

  return Status{};
}

void EfimonWorker::ProcStatsWorker(const uint delay) {
  bool first_sample = true;
  bool enabled_perf = false;
  bool enabled_samples = false;
  this->running_.store(true);

  this->mutex_.lock();
  this->cpu_usage_ =
      GetReadingsIfEnabled<CPUReadings, true>(this->proc_meter_, 0);
  this->log_table_.clear();

  enabled_perf = this->perf_annotate_meter_ != nullptr;
  if (enabled_perf) {
    this->instructions_samples_ =
        GetReadingsIfEnabled<InstructionReadings, true>(
            this->perf_annotate_meter_, 0);
  }
  enabled_samples = this->samples_ != 0;
  this->mutex_.unlock();

  EFM_CHECK(this->CreateLogTable(), EFM_WARN);
  EFM_INFO("Process with PID " + std::to_string(this->pid_) +
           " will be recorded in: " + this->name_);
  CSVLogger logger{this->name_, this->log_table_};

  while (running_.load()) {
    EFM_CHECK(RefreshProcStat(), EFM_WARN_AND_BREAK);

    // Log results
    if (first_sample) {
      first_sample = false;
    } else {
      EFM_CHECK(LogReadings(logger), EFM_WARN_AND_BREAK);
    }

    // Wait for the next sample. Perf is a blocking call
    if (enabled_perf) {
      std::this_thread::sleep_for(std::chrono::seconds(delay));
    }

    // Check if enabled samples
    if (enabled_samples && --(this->samples_) == 0) {
      running_.store(false);
    }
    EFM_INFO("Process with PID " + std::to_string(this->pid_) + " updated");
  }

  EFM_INFO("Monitoring of PID " + std::to_string(this->pid_) + " ended");
}

Status EfimonWorker::RefreshProcStat() {
  std::scoped_lock slock(this->mutex_);
  EFM_CHECK_STATUS(TriggerIfEnabled(this->proc_meter_));
  EFM_CHECK_STATUS(TriggerIfEnabled(this->perf_record_meter_));
  EFM_CHECK_STATUS(TriggerIfEnabled(this->perf_annotate_meter_));
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

#ifdef ENABLE_PERF
  if (this->perf_record_meter_ && this->perf_annotate_meter_) {
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
Status EfimonWorker::LogReadings(CSVLogger &logger) {  // NOLINT
  std::scoped_lock slock(this->mutex_);

  if (!this->cpu_usage_) {
    return Status{Status::NOT_FOUND, "Cannot find the CPU Usage"};
  }

  CPUReadings sys_cpu_readings{};
  EFM_CHECK(this->analyser_->GetReadings(3, sys_cpu_readings), EFM_WARN);

  auto timestamp = this->cpu_usage_->timestamp;
  auto difference = this->cpu_usage_->difference;
  auto proc_usage = this->cpu_usage_->overall_usage;
  auto sys_usage = sys_cpu_readings.overall_usage;

  std::unordered_map<std::string, std::shared_ptr<Logger::IValue>> values = {};

  LOG_VAL(values, "Timestamp", timestamp);
  LOG_VAL(values, "SystemCpuUsage", sys_usage);
  LOG_VAL(values, "ProcessCpuUsage", proc_usage);
  LOG_VAL(values, "TimeDifference", difference);

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

#ifdef ENABLE_PERF
  if (this->perf_record_meter_ && this->perf_annotate_meter_) {
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
        auto tit = instructions_samples_->classification.find(type);

        if (family == assembly::InstructionFamily::MEMORY ||
            family == assembly::InstructionFamily::ARITHMETIC ||
            family == assembly::InstructionFamily::LOGIC) {
          std::array<float, 4> probs;
          probs.fill(0.f);

          if (instructions_samples_->classification.end() != tit) {
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
          if (instructions_samples_->classification.end() != tit) {
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
