/**
 * @file cpuinfo.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the /proc/cpuinfo to find details about the CPU
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/proc/cpuinfo.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>

namespace efimon {

CPUInfo::CPUInfo()
    : num_logical_cores_{0},
      num_physical_cores_{0},
      num_sockets_{0},
      topology_{} {
  constexpr char kCpuInfoFile[] = "/proc/cpuinfo";
  std::ifstream proc_cpu_info_file;
  proc_cpu_info_file.open(kCpuInfoFile);

  std::string line;

  int logical_id = 0;
  int core_id = 0;
  int socket_id = 0;

  while (std::getline(proc_cpu_info_file, line)) {
    std::string::size_type idx_proc, idx_phys, idx_coreid, idx_colon;
    /* Find the processor keyword */
    idx_proc = line.find("processor");
    idx_phys = line.find("physical id");
    idx_coreid = line.find("core id");
    idx_colon = line.find(": ");
    /* Check or jump */
    bool found;
    int idx = 0;
    idx |= std::string::npos != idx_proc ? 0b1 : 0;
    idx |= std::string::npos != idx_phys ? 0b10 : 0;
    idx |= std::string::npos != idx_coreid ? 0b100 : 0;
    found = idx > 0;
    if (!found) continue;
    /* Found something - parse what is after colon KEYWORD : ^^^ */
    int payload = std::stoi(line.substr(idx_colon + 2));
    /* Classify accordingly */
    switch (idx) {
      case 0b1: /* processor */
        logical_id = payload;
        num_logical_cores_ = std::max(num_logical_cores_, logical_id + 1);
        break;
      case 0b10: /* socket id */
        socket_id = payload;
        num_sockets_ = std::max(num_sockets_, socket_id + 1);
        break;
      case 0b100: /* core id */
        core_id = payload;
        num_physical_cores_ = std::max(num_physical_cores_, core_id + 1);
        /* Flush */
        InsertMap(logical_id, socket_id, core_id);
        break;
      default:
        break;
    }
  }
}

void CPUInfo::InsertMap(const int logical_id, const int socket_id,
                        const int core_id) {
  auto it = topology_.find(socket_id);
  if (topology_.end() == it) {
    topology_[socket_id] = CPUCoreVector();
  }
  topology_[socket_id].push_back({logical_id, core_id});
}

} /* namespace efimon */
