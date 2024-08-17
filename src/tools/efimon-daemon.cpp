/**
 * @file efimon-daemon.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Daemon tool for metering the consumption of existing processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <json/json.h>
#include <atomic>
#include <efimon/arg-parser.hpp>
#include <efimon/logger/macros.hpp>
#include <efimon/power/ipmi.hpp>
#include <efimon/power/rapl.hpp>
#include <efimon/proc/cpuinfo.hpp>
#include <efimon/proc/stat.hpp>
#include <mutex>  // NOLINT
#include <sstream>
#include <unordered_map>
#include <zmqpp/zmqpp.hpp>

#include "macro-handling.hpp"  // NOLINT

using namespace efimon;  // NOLINT

static constexpr int kDelay = 3;                  // 3 seconds
static constexpr uint kDefFrequency = 100;        // 100 Hz
static constexpr uint kDefaultSampleLimit = 100;  // 100 samples
static constexpr char kDefaultOutputPath[] = "/tmp";
static constexpr uint kPort = 5550;

void print_welcome() {
  std::cout << "-----------------------------------------------------------\n";
  std::cout << "               EfiMon Daemon Application \n";
  std::cout << "-----------------------------------------------------------\n";
}

std::string create_monitoring_file(const std::string &path, const uint pid) {
  return path + "-" + std::to_string(pid) + ".csv";
}

class EfimonWorker {
 public:
  EfimonWorker() : name_{}, pid_{0}, running_{false} {};
  explicit EfimonWorker(EfimonWorker &&worker)
      : name_{std::move(worker.name_)},
        pid_{std::move(worker.pid_)},
        running_{false},
        thread_{nullptr} {
    this->running_.store(worker.running_.load());
    this->thread_.swap(worker.thread_);
  }
  EfimonWorker(const std::string &name, const uint pid)
      : name_{name}, pid_{pid}, running_{false}, thread_{nullptr} {}
  Status Start(const uint delay) {
    if (0 == this->pid_) {
      EFM_ERROR_STATUS(
          "Invalid instance of the worker. Are you using default constructor?",
          Status::CANNOT_OPEN);
    }
    EFM_INFO("Process Monitor Start for PID: " + std::to_string(this->pid_) +
             " with delay: " + std::to_string(delay));
    return Status{};
  }
  Status Stop() {
    if (0 == this->pid_) {
      EFM_ERROR_STATUS(
          "Invalid instance of the worker. Are you using default constructor?",
          Status::CANNOT_OPEN);
    }
    EFM_INFO("Process Monitor Stopped for PID: " + std::to_string(this->pid_));
    return Status{};
  }
  virtual ~EfimonWorker() = default;

 private:
  std::string name_;
  uint pid_;
  std::atomic<bool> running_;
  std::unique_ptr<std::thread> thread_;

  Status RefreshProcStat();
};

class EfimonAnalyser {
 public:
  EfimonAnalyser();

  Status StartSystemThread(const uint delay);
  Status StopSystemThread();

  Status StartWorkerThread(const std::string &name, const uint pid,
                           const uint delay);
  Status StopWorkerThread(const uint pid);

  virtual ~EfimonAnalyser() = default;

 private:
  // Running flags
  std::atomic<bool> sys_running_;
  // Meter Instances
  std::shared_ptr<Observer> proc_sys_meter_;
  std::shared_ptr<Observer> ipmi_meter_;
  std::shared_ptr<Observer> rapl_meter_;

  // Result instances
  PSUReadings *psu_readings_;
  FanReadings *fan_readings_;
  CPUReadings *cpu_energy_readings_;
  CPUReadings *cpu_usage_;

  // Refresh functions
  Status RefreshProcSys();
  Status RefreshIPMI();
  Status RefreshRAPL();

  // Workers
  void SystemStatsWorker(const int delay);

  // Running
  std::mutex sys_mutex_;
  std::unique_ptr<std::thread> sys_thread_;
  std::unordered_map<uint, EfimonWorker> proc_workers_;
};

EfimonAnalyser::EfimonAnalyser() : sys_running_{false} {
  this->ipmi_meter_ = CreateIfEnabled<IPMIMeterObserver, kEnableIpmi>();
  this->rapl_meter_ = CreateIfEnabled<RAPLMeterObserver, kEnableRapl>();
  this->proc_sys_meter_ = CreateIfEnabled<ProcStatObserver, true>(
      0, efimon::ObserverScope::SYSTEM, 1);

  // Clean up results
  this->psu_readings_ = nullptr;
  this->fan_readings_ = nullptr;
  this->cpu_energy_readings_ = nullptr;
  this->cpu_usage_ = nullptr;
}

Status EfimonAnalyser::StartSystemThread(const uint delay) {
  if (nullptr != this->sys_thread_) {
    return Status{Status::RESOURCE_BUSY, "The thread has already started"};
  }

  EFM_INFO("Starting System Monitor");

  this->sys_running_.store(true);
  this->sys_thread_ = std::make_unique<std::thread>(
      &EfimonAnalyser::SystemStatsWorker, this, delay);
  return Status{};
}

Status EfimonAnalyser::StartWorkerThread(const std::string &name,
                                         const uint pid, const uint delay) {
  if (this->proc_workers_.end() != this->proc_workers_.find(pid)) {
    return Status{Status::RESOURCE_BUSY,
                  "The monitor has already started for the given PID: " +
                      std::to_string(pid)};
  }

  EFM_INFO("Creating Process Monitor for PID: " + std::to_string(pid));
  auto pair = this->proc_workers_.emplace(pid, EfimonWorker{name, pid});

  if (!pair.second) {
    return Status{
        Status::CANNOT_OPEN,
        "Cannot create the monitor for the given PID: " + std::to_string(pid)};
  }

  EFM_INFO("Starting Process Monitor for PID: " + std::to_string(pid));
  return this->proc_workers_[pid].Start(delay);
}

Status EfimonAnalyser::StopSystemThread() {
  if (nullptr == this->sys_thread_) {
    return Status{Status::NOT_FOUND, "The thread was not running"};
  }

  EFM_INFO("Stopping System Monitor");

  this->sys_running_.store(false);
  this->sys_thread_->join();
  this->sys_thread_.reset();
  return Status{};
}

Status EfimonAnalyser::StopWorkerThread(const uint pid) {
  auto it = this->proc_workers_.find(pid);
  if (this->proc_workers_.end() == it) {
    return Status{Status::NOT_FOUND,
                  "No monitor linked to the given PID: " + std::to_string(pid)};
  }

  EFM_INFO("Stopping Worker Monitor for PID: " + std::to_string(pid));

  it->second.Stop();
  this->proc_workers_.erase(it);
  return Status{};
}

Status EfimonAnalyser::RefreshIPMI() {
#ifdef ENABLE_IPMI
  std::scoped_lock slock(this->sys_mutex_);
#endif
  return Status{};
}

Status EfimonAnalyser::RefreshRAPL() {
#ifdef ENABLE_RAPL
  std::scoped_lock slock(this->sys_mutex_);
  return Status{};
#endif
  return Status{};
}

Status EfimonAnalyser::RefreshProcSys() {
  std::scoped_lock slock(this->sys_mutex_);
  return Status{};
}

void EfimonAnalyser::SystemStatsWorker(const int delay) {
  sys_running_.store(true);

  this->sys_mutex_.lock();
  this->psu_readings_ =
      GetReadingsIfEnabled<PSUReadings, kEnableIpmi>(this->ipmi_meter_, 0);
  this->fan_readings_ =
      GetReadingsIfEnabled<FanReadings, kEnableIpmi>(this->ipmi_meter_, 1);
  this->cpu_energy_readings_ =
      GetReadingsIfEnabled<CPUReadings, kEnableRapl>(this->rapl_meter_, 0);
  this->cpu_usage_ =
      GetReadingsIfEnabled<CPUReadings, true>(this->proc_sys_meter_, 0);
  this->sys_mutex_.unlock();

  while (sys_running_.load()) {
    EFM_CHECK(RefreshProcSys(), EFM_WARN);
    EFM_CHECK(RefreshIPMI(), EFM_WARN);
    EFM_CHECK(RefreshRAPL(), EFM_WARN);

    /* Wait for the next sample */
    std::this_thread::sleep_for(std::chrono::seconds(delay));
    EFM_INFO("System Updated");
  }
}

int main(int argc, char **argv) {
  print_welcome();
  // ------------ Configuration variables ------------
  uint frequency = kDefFrequency;
  uint samples = kDefaultSampleLimit;
  uint delaytime = kDelay;
  std::string outputpath = kDefaultOutputPath;
  uint port = kPort;

  // ------------ Arguments ------------
  ArgParser argparser(argc, argv);

  bool check_frequency =
      argparser.Exists("-f") || argparser.Exists("--frequency");
  bool check_samples = argparser.Exists("-s") || argparser.Exists("--samples");
  bool check_delay = argparser.Exists("-d") || argparser.Exists("--delay");
  bool check_output =
      argparser.Exists("-o") || argparser.Exists("--output-folder");
  bool check_help = argparser.Exists("-h") || argparser.Exists("--help");
  bool check_port = argparser.Exists("-p") || argparser.Exists("--port");

  if (check_help) {
    std::string msg =
        "This application launches a daemon listener for measuring external "
        "applications: EfiMon Daemon\n\tUsage: "
        "\n\t";
    msg += std::string(argv[0]);
    msg +=
        " -s,--samples SAMPLES (default: 100). Number of samples to "
        "collect\n\t\t";
    msg +=
        " -o,--output-folder PATH (default: /tmp). Output folder to save "
        "measurements\n\t\t";
    msg +=
        " -f,--frequency FREQUENCY_HZ (default: 100 Hz). Sampling "
        "frequency\n\t\t";
    msg +=
        " -d,--delay DELAY_SECS (default: 3 Secs). Sampling time window\n\t\t";
    msg +=
        " -p,--port PORT (default: 5550 Secs). EfiMon Socket Port for "
        "IPC\n\t\t";
    msg += " -h,--help: prints this message\n\n";
    msg +=
        " \tBy default, the outputs will be saved into the folder with the "
        "pattern output-pid.csv\n";
    EFM_ERROR(msg);
  }

  // ------------ Modify the configurations -----------
  if (check_samples) {
    samples =
        std::stoi(argparser.Exists("-s") ? argparser.GetOption("-s")
                                         : argparser.GetOption("--samples"));
  }

  if (check_frequency) {
    frequency =
        std::stoi(argparser.Exists("-f") ? argparser.GetOption("-f")
                                         : argparser.GetOption("--frequency"));
  }

  if (check_delay) {
    delaytime =
        std::stoi(argparser.Exists("-d") ? argparser.GetOption("-d")
                                         : argparser.GetOption("--delay"));
  }

  if (check_port) {
    port = std::stoi(argparser.Exists("-p") ? argparser.GetOption("-p")
                                            : argparser.GetOption("--port"));
  }

  if (check_output) {
    outputpath = argparser.Exists("-o")
                     ? argparser.GetOption("-o")
                     : argparser.GetOption("--output-folder");
  }

  EFM_INFO(std::string("Frequency [Hz]: ") + std::to_string(frequency));
  EFM_INFO(std::string("Samples: ") + std::to_string(samples));
  EFM_INFO(std::string("Delay time [secs]: ") + std::to_string(delaytime));
  EFM_INFO(std::string("Output folder: ") + outputpath);
  EFM_INFO(std::string("IPC TCP Port: ") + std::to_string(port));

  // ---------- Initialise ZeroMQ ------------
  std::string endpoint = "tcp://*:" + std::to_string(port);
  zmqpp::context context;
  zmqpp::socket_type type = zmqpp::socket_type::reply;
  zmqpp::socket socket{context, type};
  socket.bind(endpoint);

  // ----------- Start the thread -----------
  EfimonAnalyser analyser{};
  analyser.StartSystemThread(delaytime);

  // ----------- Listen forever -----------
  Json::CharReaderBuilder rbuilder;
  Json::Value root;
  rbuilder["collectComments"] = false;

  while (true) {
    zmqpp::message message;
    std::string text, errs;
    std::stringstream streamtext;

    // Receive message
    socket.receive(message);
    message >> text;
    streamtext << text;

    // Do some work
    bool ok = Json::parseFromStream(rbuilder, streamtext, &root, &errs);
    if (!ok) {
      EFM_WARN("Error while Json: " + errs);
      socket.send("{\"result\": \"Cannot parse\"}");
    } else {
      Status status{Status::OK, "OK"};

      // Check transaction
      if (!root.isMember("transaction")) {
        EFM_WARN("'transaction' member does not exist");
        socket.send("{\"result\": \"Cannot find transaction\"}");
      }
      std::string transaction = root["transaction"].asString();
      EFM_INFO("Transaction: " + transaction);

      // Complete transaction
      if ("system" == transaction && root.isMember("state")) {
        bool state = root["state"].asBool();
        EFM_INFO("Setting System Monitor to: " + std::to_string(state));
        if (state) {
          status = analyser.StartSystemThread(delaytime);
        } else {
          status = analyser.StopSystemThread();
        }
      } else if ("process" == transaction && root.isMember("state") &&
                 root.isMember("pid")) {
        bool state = root["state"].asBool();
        uint pid = root["pid"].asUInt();
        uint delay =
            root.isMember("delay") ? root["delay"].asUInt() : delaytime;
        std::string name =
            root.isMember("name") ? root["name"].asString() : "Unknown";
        EFM_INFO("Setting Process Monitor to PID " + std::to_string(pid) +
                 " to: " + std::to_string(state) +
                 " with delay: " + std::to_string(delay) + " secs");
        if (state) {
          status = analyser.StartWorkerThread(name, pid, delay);
        } else {
          status = analyser.StopWorkerThread(pid);
        }
      } else {
        status = Status{Status::INVALID_PARAMETER, "Invalid set of params"};
      }

      if (Status::OK != status.code) {
        EFM_WARN(status.what());
      }

      socket.send("{\"result\": \"" + std::string(status.what()) + "\"}");
    }
  }

  return 0;
}
