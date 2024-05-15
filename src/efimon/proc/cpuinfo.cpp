/**
 * @file cpuinfo.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the /proc/cpuinfo to find details about the CPU
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <algorithm>
#include <efimon/proc/cpuinfo.hpp>
#include <fstream>
#include <mutex>  // NOLINT
#include <sstream>
#include <string>
#include <tuple>

namespace efimon {

static constexpr char kCpuInfoFile[] = "/proc/cpuinfo";
static std::mutex m_single_cpuinfo;

CPUInfo::CPUInfo()
    : num_logical_cores_{0},
      num_physical_cores_{0},
      num_sockets_{0},
      topology_{} {
  /* Parses the map and constructs it */
  ParseMap();

  /* Comparison predicate: order by logical core according to the
     efimon::CPUPair definition */
  auto compare_pair = [](CPUPair a, CPUPair b) {
    return std::get<0>(a) < std::get<0>(b);
  };

  /* Iterate over the sockets */
  for (auto& socket_pair : topology_) {
    auto& socket_vector = socket_pair.second;
    std::sort(socket_vector.begin(), socket_vector.end(), compare_pair);
  }
}

void CPUInfo::InsertMap(const int logical_id, const int socket_id,
                        const int core_id, const float clock_speed) {
  auto it = topology_.find(socket_id);
  if (topology_.end() == it) {
    topology_[socket_id] = CPUCoreVector();
  }
  topology_[socket_id].push_back({logical_id, core_id, clock_speed});
}

const CPUInfo::CPUAssignment& CPUInfo::GetAssignation() {
  std::scoped_lock lock(m_single_cpuinfo);
  return topology_;
}

void CPUInfo::ParseMap() {
  std::ifstream proc_cpu_info_file;
  proc_cpu_info_file.open(kCpuInfoFile);

  std::string line;

  int logical_id = 0;
  int core_id = 0;
  int socket_id = 0;
  float clock_mhz = 0;

  while (std::getline(proc_cpu_info_file, line)) {
    std::scoped_lock lock(m_single_cpuinfo);
    std::string::size_type idx_proc, idx_phys, idx_coreid, idx_clock, idx_colon;
    /* Find the processor keyword */
    idx_proc = line.find("processor");
    idx_phys = line.find("physical id");
    idx_coreid = line.find("core id");
    idx_clock = line.find("cpu MHz");
    idx_colon = line.find(": ");
    /* Check or jump */
    bool found;
    int idx = 0;
    idx |= std::string::npos != idx_proc ? 0b1 : 0;
    idx |= std::string::npos != idx_clock ? 0b10 : 0;
    idx |= std::string::npos != idx_phys ? 0b100 : 0;
    idx |= std::string::npos != idx_coreid ? 0b1000 : 0;
    found = idx > 0;
    if (!found) continue;
    /* Found something - parse what is after colon KEYWORD : ^^^ */
    /* Classify accordingly */
    switch (idx) {
      case 0b1: /* processor */
        logical_id = std::stoi(line.substr(idx_colon + 2));
        num_logical_cores_ = std::max(num_logical_cores_, logical_id + 1);
        break;
      case 0b10: /* clock speed */
        clock_mhz = std::stof(line.substr(idx_colon + 2));
        break;
      case 0b100: /* socket id */
        socket_id = std::stoi(line.substr(idx_colon + 2));
        num_sockets_ = std::max(num_sockets_, socket_id + 1);
        break;
      case 0b1000: /* core id */
        core_id = std::stoi(line.substr(idx_colon + 2));
        num_physical_cores_ = std::max(num_physical_cores_, core_id + 1);
        /* Flush */
        InsertMap(logical_id, socket_id, core_id, clock_mhz);
        break;
      default:
        break;
    }
  }
}

Status CPUInfo::Refresh() {
  topology_.clear();
  ParseMap();
  return Status{};
}

float CPUInfo::GetMeanFrequency() {
  float sum = 0.f;
  std::vector<float> means = this->GetSocketMeanFrequency();
  for (const float mean : means) {
    sum += mean;
  }
  return sum / static_cast<float>(this->num_sockets_);
}

std::vector<float> CPUInfo::GetSocketMeanFrequency() {
  std::vector<float> socket_means(this->num_sockets_);

  for (int i = 0; i < this->num_sockets_; ++i) {
    float sum = 0.f;
    for (const auto& tuple : this->topology_[i]) {
      sum += std::get<2>(tuple);
    }
    socket_means[i] = sum / static_cast<float>(this->topology_[i].size());
  }

  return socket_means;
}

} /* namespace efimon */
