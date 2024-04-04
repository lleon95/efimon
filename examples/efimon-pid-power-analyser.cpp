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

#include <efimon/arg-parser.hpp>
#include <efimon/asm-classifier.hpp>
#include <efimon/perf/annotate.hpp>
#include <efimon/perf/record.hpp>
#include <efimon/power/ipmi.hpp>
#include <efimon/power/rapl.hpp>
#include <efimon/proc/stat.hpp>
#include <iostream>
#include <string>

#define EFM_INFO(msg) \
  { std::cerr << "[INFO]: " << msg << std::endl; }
#define EFM_WARN(msg) \
  { std::cerr << "[WARNING]: " << msg << std::endl; }
#define EFM_ERROR(msg)                            \
  {                                               \
    std::cerr << "[ERROR]: " << msg << std::endl; \
    return -1;                                    \
  }
#define EFM_CHECK(inst)                           \
  {                                               \
    Status s_ = (inst);                           \
    if (s_.code != Status::OK) EFM_ERROR(s_.msg); \
  }
#define EFM_RES std::cout

using namespace efimon;  // NOLINT

static constexpr int kDelay = 1;         // 1 second
static constexpr int kFrequency = 1000;  // 1 kHz

int main(int argc, char **argv) {
  // Check root
  if (0 != geteuid()) {
    EFM_ERROR("ERROR: This application must be called as root" << std::endl);
  }

  // Check arguments
  ArgParser argparser(argc, argv);
  if (argc < 5 || !(argparser.Exists("-p") || argparser.Exists("--pid"))) {
    std::string msg =
        "ERROR: This command requires the PID and SAMPLES to analyse\n\tUsage: "
        "\n\t";
    msg += std::string(argv[0]);
    msg += " -p,--pid PID";
    msg += " -s,--samples SAMPLES";
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

  // Extracting the PID
  uint pid = std::stoi(argparser.Exists("-p") ? argparser.GetOption("-p")
                                              : argparser.GetOption("--pid"));
  uint timelimit =
      std::stoi(argparser.Exists("-s") ? argparser.GetOption("-s")
                                       : argparser.GetOption("--samples"));
  EFM_INFO(std::string("Analysing PID ") + std::to_string(pid));

  // ------------ Configure all tools ------------
#ifdef ENABLE_IPMI
  EFM_INFO("Configuring IPMI");
  IPMIMeterObserver ipmi_meter{};
  auto psu_readings_iface = ipmi_meter.GetReadings()[0];
  PSUReadings *psu_readings = dynamic_cast<PSUReadings *>(psu_readings_iface);
  EFM_CHECK(ipmi_meter.Trigger());
  uint psu_num = psu_readings->psu_max_power.size();
  EFM_INFO("PSUs detected: " + std::to_string(psu_num));
#endif
#ifdef ENABLE_RAPL
  EFM_INFO("Configuring RAPL");
  RAPLMeterObserver rapl_meter{};
  auto rapl_readings_iface = rapl_meter.GetReadings()[0];
  CPUReadings *rapl_readings = dynamic_cast<CPUReadings *>(rapl_readings_iface);
  EFM_CHECK(rapl_meter.Trigger());
  uint socket_num = rapl_readings->socket_power.size();
  EFM_INFO("Sockets detected: " + std::to_string(socket_num));
#endif
#ifdef ENABLE_PERF
  EFM_INFO("Configuring PERF");
  PerfRecordObserver perf_record{pid, ObserverScope::PROCESS, kDelay,
                                 kFrequency, true};
  PerfAnnotateObserver perf_annotate{perf_record};
#endif
  ProcStatObserver proc_stat{pid, efimon::ObserverScope::PROCESS, 1};
  ProcStatObserver sys_stat{0, efimon::ObserverScope::SYSTEM, 1};
  auto proc_stat_iface = proc_stat.GetReadings()[0];
  auto sys_stat_iface = sys_stat.GetReadings()[0];
  CPUReadings *proc_cpu_usage = dynamic_cast<CPUReadings *>(proc_stat_iface);
  CPUReadings *sys_cpu_usage = dynamic_cast<CPUReadings *>(sys_stat_iface);
  EFM_CHECK(proc_stat.Trigger());
  EFM_CHECK(sys_stat.Trigger());

  // ------------ Making table header ------------
  EFM_INFO("Readings:");
  EFM_RES << "Timestamp"
          << ",";
#ifdef ENABLE_IPMI
  for (uint i = 0; i < psu_num; ++i) {
    EFM_RES << "PSUPower" << i << ",";
  }
#endif
#ifdef ENABLE_RAPL
  for (uint i = 0; i < socket_num; ++i) {
    EFM_RES << "SocketPower" << i << ",";
  }
#endif
#ifdef ENABLE_PERF
  for (uint itype = 0;
       itype < static_cast<uint>(assembly::InstructionType::UNCLASSIFIED);
       ++itype) {
    for (uint ftype = 0;
         ftype < static_cast<uint>(assembly::InstructionFamily::OTHER);
         ++ftype) {
      auto type = static_cast<assembly::InstructionType>(itype);
      auto family = static_cast<assembly::InstructionFamily>(ftype);
      std::string stype = AsmClassifier::TypeString(type);
      std::string sfamily = AsmClassifier::FamilyString(family);
      EFM_RES << "Probability" << stype << sfamily << ",";
    }
  }
#endif
  EFM_RES << "SystemCpuUsage"
          << ",";
  EFM_RES << "ProcessCpuUsage"
          << ",";
  EFM_RES << "TimeDifference" << std::endl;
  // ------------ Perform reads ------------
  bool first = true;
  for (uint t = 0; t < timelimit; ++t) {
    EFM_CHECK(proc_stat.Trigger());
    EFM_CHECK(sys_stat.Trigger());
#ifdef ENABLE_PERF
    EFM_CHECK(perf_record.Trigger());
    EFM_CHECK(perf_annotate.Trigger());
    auto readings_rec =
        dynamic_cast<RecordReadings *>(perf_record.GetReadings()[0]);
    auto readings_ann =
        dynamic_cast<InstructionReadings *>(perf_annotate.GetReadings()[0]);
#else
    sleep(kDelay);
#endif
#ifdef ENABLE_RAPL
    EFM_CHECK(rapl_meter.Trigger());
#endif
#ifdef ENABLE_IPMI
    EFM_CHECK(ipmi_meter.Trigger());
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
         itype < static_cast<uint>(assembly::InstructionType::UNCLASSIFIED);
         ++itype) {
      for (uint ftype = 0;
           ftype < static_cast<uint>(assembly::InstructionFamily::OTHER);
           ++ftype) {
        auto type = static_cast<assembly::InstructionType>(itype);
        auto family = static_cast<assembly::InstructionFamily>(ftype);
        auto tit = readings_ann->classification.find(type);
        if (readings_ann->classification.end() == tit) {
          EFM_RES << 0.f << ",";
          continue;
        }
        auto fit = tit->second.find(family);
        if (tit->second.end() == fit) {
          EFM_RES << 0.f << ",";
          continue;
        }
        EFM_RES << fit->second << ",";
      }
    }
#endif

    // PSU columns
#ifdef ENABLE_IPMI
    for (uint i = 0; i < psu_num; ++i) {
      EFM_RES << psu_readings->psu_power.at(i) << ",";
    }
#endif
    EFM_RES << sys_cpu_usage->overall_usage << ",";
    EFM_RES << proc_cpu_usage->overall_usage << ",";
    EFM_RES << difference << std::endl;
  }

  return 0;
}
