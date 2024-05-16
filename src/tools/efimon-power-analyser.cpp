/**
 * @file efimon-pid-power-analyser.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @copyright Copyright (c) 2024. See License for Licensing
 *
 * @brief Efimon Power Analyser Tool.
 *
 * This tool extracts the CPU power
 * consumption, the PSU report and the histogram of instruction distribution
 * of a process to have an approximate of its power consumption
 *
 * This tool depends on:
 *
 * - Linux Perf
 * - RAPL
 * - IPMI
 *
 * Moreover, it requires sudo permissions. Currently, the support
 * is limited to Dell Motherboards with AMD EPYC Zen2 Processors.
 *
 * This tool is not capable of exploring other subprocesses
 */

#include <unistd.h>

#include <array>
#include <condition_variable>  // NOLINT
#include <efimon/arg-parser.hpp>
#include <efimon/asm-classifier.hpp>
#include <efimon/perf/annotate.hpp>
#include <efimon/perf/record.hpp>
#include <efimon/power/ipmi.hpp>
#include <efimon/power/rapl.hpp>
#include <efimon/proc/cpuinfo.hpp>
#include <efimon/proc/stat.hpp>
#include <efimon/process-manager.hpp>
#include <iostream>
#include <mutex>  // NOLINT
#include <string>
#include <third-party/pstream.hpp>
#include <thread>  // NOLINT

#define EFM_INFO(msg) \
  { std::cerr << "[INFO]: " << msg << std::endl; }
#define EFM_WARN(msg) \
  { std::cerr << "[WARNING]: " << msg << std::endl; }
#define EFM_WARN_AND_BREAK(msg)                     \
  {                                                 \
    std::cerr << "[WARNING]: " << msg << std::endl; \
    break;                                          \
  }
#define EFM_ERROR(msg)                            \
  {                                               \
    std::cerr << "[ERROR]: " << msg << std::endl; \
    return -1;                                    \
  }
#define EFM_CHECK(inst, func)                \
  {                                          \
    Status s_ = (inst);                      \
    if (s_.code != Status::OK) func(s_.msg); \
  }
#define EFM_CRITICAL_CHECK(inst)                  \
  {                                               \
    Status s_ = (inst);                           \
    if (s_.code != Status::OK) EFM_ERROR(s_.msg); \
  }
#define EFM_RES std::cout

using namespace efimon;  // NOLINT

static constexpr int kDelay = 1;                // 1 second
static constexpr uint kDefFrequency = 100;      // 100 Hz
static constexpr int kThreadCheckTime = 10;     // 10 millis
static constexpr uint kDefaultTimelimit = 100;  // 100 samples

void launch_command(ProcessManager &proc,                        // NOLINT
                    const std::vector<std::string> &args,        // NOLINT
                    std::mutex &m, std::condition_variable &cv,  // NOLINT
                    bool &close,                                 // NOLINT
                    bool &terminated) {                          // NOLINT
  // Create the process and launch it
  Status st;
  uint count = args.size();

  m.lock();
  if (count == 1) {
    st = proc.Open(args[0], ProcessManager::Mode::SILENT);
  } else {
    st = proc.Open(args[0], args, ProcessManager::Mode::SILENT);
  }
  m.unlock();

  if (Status::OK != st.code) {
    m.lock();
    terminated = true;
    m.unlock();
    cv.notify_one();
    return;
  }
  cv.notify_one();

  bool local_close = false;
  while (!local_close) {
    if (!proc.IsRunning()) {
      m.lock();
      terminated = true;
      m.unlock();
      break;
    }

    // Sleep while running
    std::this_thread::sleep_for(std::chrono::milliseconds(kThreadCheckTime));

    // Check close status
    m.lock();
    local_close = close;
    m.unlock();
  }
  m.lock();
  proc.Close();
  m.unlock();
}

int main(int argc, char **argv) {
  // Analyser control
  uint frequency = kDefFrequency;
  uint timelimit = kDefaultTimelimit;
  uint pid = 0;

  // Process management
  std::thread manager_thread;
  ProcessManager manager;
  bool terminated = false;
  bool close = false;
  std::mutex manager_mutex;
  std::vector<std::string> manager_args;
  std::condition_variable manager_cv;

  // Check root
  if (0 != geteuid()) {
    EFM_ERROR("ERROR: This application must be called as root");
  }

  // ------------ Arguments ------------
  ArgParser argparser(argc, argv);
  bool check_pid = argparser.Exists("-p") || argparser.Exists("--pid");
  bool check_cmd = argparser.Exists("-c");
  bool check_frequency =
      argparser.Exists("-f") || argparser.Exists("--frequency");
  bool check_samples = argparser.Exists("-s") || argparser.Exists("--samples");
  bool check_process = check_cmd ^ check_pid;
  if (argc < 3 || !(check_process)) {
    std::string msg =
        "ERROR: This command requires the PID and SAMPLES to analyse\n\tUsage: "
        "\n\t";
    msg += std::string(argv[0]);
    msg += " -p,--pid PID";
    msg += " -s,--samples SAMPLES (default: 100)";
    msg += " -f,--frequency FREQUENCY_HZ (default: 100 Hz)";
    msg += " -c [COMMAND]\n\t";
    msg += " -p and -c are mutually exclusive. -c goes to the end always!";
    EFM_ERROR(msg);
  }

  // Check tools
#ifdef ENABLE_IPMI
  EFM_INFO("IPMI found. Enabling");
#else
  EFM_WARN("IPMI not found.");
#endif
#ifdef ENABLE_PERF
  EFM_INFO("PERF found. Enabling");
#else
  EFM_WARN("PERF not found.");
#endif
#ifdef ENABLE_RAPL
  EFM_INFO("RAPL found. Enabling");
#else
  EFM_WARN("RAPL not found.");
#endif

  // ------------ Argument handling ------------
  if (check_pid) {
    pid = std::stoi(argparser.Exists("-p") ? argparser.GetOption("-p")
                                           : argparser.GetOption("--pid"));
  } else {
    // Get the arguments from the command
    auto bit = argparser.GetBegin("-c");
    auto eit = argparser.GetEnd();
    const int count = eit - bit;
    manager_args.resize(count);
    std::copy(bit, eit, manager_args.begin());

    // Launch the thread
    EFM_INFO("Launching the process");
    manager_thread =
        std::thread(launch_command, std::ref(manager), std::cref(manager_args),
                    std::ref(manager_mutex), std::ref(manager_cv),
                    std::ref(close), std::ref(terminated));
    {
      std::unique_lock lk(manager_mutex);
      manager_cv.wait_for(lk, std::chrono::seconds(1));
    }
    EFM_INFO("Checking the launch");

    // Check if it is open
    bool running = false;
    manager_mutex.lock();
    running = !terminated;
    pid = manager.GetPID();
    manager_mutex.unlock();
    if (!running) {
      EFM_ERROR(std::string("Cannot run the command: ") + *bit);
    }

    EFM_INFO("Launched successfully");
  }

  // Extract the timelimit
  if (check_samples) {
    timelimit =
        std::stoi(argparser.Exists("-s") ? argparser.GetOption("-s")
                                         : argparser.GetOption("--samples"));
  }

  // Extract the frequency
  if (check_frequency) {
    frequency =
        std::stoi(argparser.Exists("-f") ? argparser.GetOption("-f")
                                         : argparser.GetOption("--frequency"));
  }

  EFM_INFO(std::string("Analysing PID ") + std::to_string(pid));
  EFM_INFO(std::string("Frequency: ") + std::to_string(frequency));
  EFM_INFO(std::string("Samples: ") + std::to_string(timelimit));

  // ------------ Configure all tools ------------
#ifdef ENABLE_IPMI
  EFM_INFO("Configuring IPMI");
  IPMIMeterObserver ipmi_meter{};
  auto psu_readings_iface = ipmi_meter.GetReadings()[0];
  auto fan_readings_iface = ipmi_meter.GetReadings()[1];
  PSUReadings *psu_readings = dynamic_cast<PSUReadings *>(psu_readings_iface);
  FanReadings *fan_readings = dynamic_cast<FanReadings *>(fan_readings_iface);
  EFM_CRITICAL_CHECK(ipmi_meter.Trigger());
  uint psu_num = psu_readings->psu_max_power.size();
  uint fan_num = fan_readings->fan_speeds.size();
  EFM_INFO("PSUs detected: " + std::to_string(psu_num));
  EFM_INFO("Fans detected: " + std::to_string(fan_num));
#ifndef ENABLE_IPMI_SENSORS
  EFM_WARN("IPMI Sensors not found. Skipping fan measurements");
#endif
#endif
#ifdef ENABLE_RAPL
  EFM_INFO("Configuring RAPL");
  RAPLMeterObserver rapl_meter{};
  auto rapl_readings_iface = rapl_meter.GetReadings()[0];
  CPUReadings *rapl_readings = dynamic_cast<CPUReadings *>(rapl_readings_iface);
  EFM_CRITICAL_CHECK(rapl_meter.Trigger());
  uint socket_num = rapl_readings->socket_power.size();
  EFM_INFO("Sockets detected: " + std::to_string(socket_num));
#endif
#ifdef ENABLE_PERF
  EFM_INFO("Configuring PERF");
  PerfRecordObserver perf_record{pid, ObserverScope::PROCESS, kDelay, frequency,
                                 true};
  PerfAnnotateObserver perf_annotate{perf_record};
#endif
  ProcStatObserver proc_stat{pid, efimon::ObserverScope::PROCESS, 1};
  ProcStatObserver sys_stat{0, efimon::ObserverScope::SYSTEM, 1};
  auto proc_stat_iface = proc_stat.GetReadings()[0];
  auto sys_stat_iface = sys_stat.GetReadings()[0];
  CPUReadings *proc_cpu_usage = dynamic_cast<CPUReadings *>(proc_stat_iface);
  CPUReadings *sys_cpu_usage = dynamic_cast<CPUReadings *>(sys_stat_iface);
  EFM_CRITICAL_CHECK(proc_stat.Trigger());
  EFM_CRITICAL_CHECK(sys_stat.Trigger());
  CPUInfo cpuinfo{};

  // ------------ Making table header ------------
  EFM_INFO("Readings:");
  EFM_RES << "Timestamp"
          << ",";
#ifdef ENABLE_RAPL
  for (uint i = 0; i < socket_num; ++i) {
    EFM_RES << "SocketPower" << i << ",";
  }
#endif
#ifdef ENABLE_PERF
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
        EFM_RES << "ProbabilityRegister" << stype << sfamily << ",";
        EFM_RES << "ProbabilityMemLoad" << stype << sfamily << ",";
        EFM_RES << "ProbabilityMemStore" << stype << sfamily << ",";
        EFM_RES << "ProbabilityMemUpdate" << stype << sfamily << ",";
      } else {
        EFM_RES << "Probability" << stype << sfamily << ",";
      }
    }
  }
#endif
#ifdef ENABLE_IPMI
  for (uint i = 0; i < psu_num; ++i) {
    EFM_RES << "PSUPower" << i << ",";
  }
  for (uint i = 0; i < fan_num; ++i) {
    EFM_RES << "FanSpeed" << i << ",";
  }
#endif
  for (int i = 0; i < cpuinfo.GetNumSockets(); ++i) {
    EFM_RES << "SocketFreq" << i << ",";
  }
  EFM_RES << "SystemCpuUsage"
          << ",";
  EFM_RES << "ProcessCpuUsage"
          << ",";
  EFM_RES << "TimeDifference" << std::endl;
  // ------------ Perform reads ------------
  bool first = true;
  for (uint t = 0; t < timelimit; ++t) {
    std::cout << std::flush;
    bool finished = false;
    manager_mutex.lock();
    finished = terminated;
    manager_mutex.unlock();
    if (finished) EFM_WARN_AND_BREAK("Process not running");

    EFM_CHECK(proc_stat.Trigger(), EFM_WARN_AND_BREAK);
    EFM_CHECK(sys_stat.Trigger(), EFM_WARN_AND_BREAK);
    EFM_CHECK(cpuinfo.Refresh(), EFM_WARN_AND_BREAK);
#ifdef ENABLE_PERF
    EFM_CHECK(perf_record.Trigger(), EFM_WARN_AND_BREAK);
    EFM_CHECK(perf_annotate.Trigger(), EFM_WARN_AND_BREAK);
    auto readings_rec =
        dynamic_cast<RecordReadings *>(perf_record.GetReadings()[0]);
    auto readings_ann =
        dynamic_cast<InstructionReadings *>(perf_annotate.GetReadings()[0]);
#else
    sleep(kDelay);
#endif
#ifdef ENABLE_RAPL
    EFM_CHECK(rapl_meter.Trigger(), EFM_WARN_AND_BREAK);
#endif
#ifdef ENABLE_IPMI
    EFM_CHECK(ipmi_meter.Trigger(), EFM_WARN_AND_BREAK);
#endif

    // Time columns
#ifdef ENABLE_PERF
    auto timestamp = readings_rec->timestamp;
    auto difference = readings_rec->difference;
#else
    auto timestamp = sys_cpu_usage->timestamp;
    auto difference = sys_cpu_usage->difference;
#endif
    // Discard first
    if (first) {
      first = false;
      continue;
    }
    EFM_RES << timestamp << ",";

#ifdef ENABLE_RAPL
    for (uint i = 0; i < socket_num; ++i) {
      EFM_RES << (rapl_readings->socket_power.at(i) * 1e3 / difference) << ",";
    }
#endif

#ifdef ENABLE_PERF
    for (uint itype = 0;
         itype <= static_cast<uint>(assembly::InstructionType::UNCLASSIFIED);
         ++itype) {
      for (uint ftype = 0;
           ftype < static_cast<uint>(assembly::InstructionFamily::OTHER);
           ++ftype) {
        auto type = static_cast<assembly::InstructionType>(itype);
        auto family = static_cast<assembly::InstructionFamily>(ftype);

        auto tit = readings_ann->classification.find(type);

        if (family == assembly::InstructionFamily::MEMORY ||
            family == assembly::InstructionFamily::ARITHMETIC ||
            family == assembly::InstructionFamily::LOGIC) {
          std::array<float, 4> probs;
          probs.fill(0.f);

          if (readings_ann->classification.end() != tit) {
            auto fit = tit->second.find(family);
            if (tit->second.end() != fit) {
              for (auto origit = fit->second.begin();
                   origit != fit->second.end(); origit++) {
                auto pairorigin =
                    AsmClassifier::OriginDecomposed(origit->first);
                if (pairorigin.first == assembly::DataOrigin::MEMORY &&
                    pairorigin.second == assembly::DataOrigin::MEMORY) {
                  probs[3] += origit->second;
                } else if (pairorigin.first == assembly::DataOrigin::MEMORY) {
                  probs[1] += origit->second;
                } else if (pairorigin.second == assembly::DataOrigin::MEMORY) {
                  probs[2] += origit->second;
                } else {
                  probs[0] += origit->second;
                }
              }
            }
          }

          for (const auto prob : probs) {
            EFM_RES << prob << ",";
          }
        } else {
          float probres = 0.f;
          if (readings_ann->classification.end() != tit) {
            auto fit = tit->second.find(family);
            if (tit->second.end() != fit) {
              for (auto origit = fit->second.begin();
                   origit != fit->second.end(); origit++) {
                probres += origit->second;
              }
            }
          }
          EFM_RES << probres << ",";
        }
      }
    }
#endif

    // PSU columns
#ifdef ENABLE_IPMI
    for (uint i = 0; i < psu_num; ++i) {
      EFM_RES << psu_readings->psu_power.at(i) << ",";
    }
    for (uint i = 0; i < fan_num; ++i) {
      EFM_RES << fan_readings->fan_speeds.at(i) << ",";
    }
#endif
    for (const float freq : cpuinfo.GetSocketMeanFrequency()) {
      EFM_RES << freq << ",";
    }
    EFM_RES << sys_cpu_usage->overall_usage << ",";
    EFM_RES << proc_cpu_usage->overall_usage << ",";
    EFM_RES << difference << std::endl;
  }

  if (check_cmd) {
    EFM_INFO("Sending termination signal");
    // Order closure
    manager_mutex.lock();
    close = true;
    manager_mutex.unlock();
    kill(pid, SIGINT);
    // Wait until closure
    manager_thread.join();
  }

  EFM_INFO("Finished...");

  return 0;
}
