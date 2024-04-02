/**
 * @file cpuinfo.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the /proc/cpuinfo to find details about the CPU
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PROC_CPUINFO_HPP_
#define INCLUDE_EFIMON_PROC_CPUINFO_HPP_

#include <unordered_map>
#include <utility>
#include <vector>

#include <efimon/status.hpp>

namespace efimon {

class CPUInfo {
 public:
  /**
   * @brief Defines the tuple for the CPU pair
   *
   * The first attribute is the logical id and the second is the physical core
   * in case of SMT enabled.
   */
  typedef std::pair<int, int> CPUPair;

  /**
   * @brief Defines the vector of tuples for the CPU pair
   */
  typedef std::vector<CPUPair> CPUCoreVector;

  /**
   * @brief Defines the topology map where the socket is analysed in terms of
   * its logical and physical cores
   *
   * The key is the socket id whereas the attribute is a CPUPaur
   */
  typedef std::unordered_map<int, CPUCoreVector> CPUAssignment;

  /**
   * @brief Construct a new CPUInfo
   *
   * It opens the /proc/cpuinfo file, parses it and fills all the attributes
   * of the object, including the counters and the topology map
   */
  CPUInfo();

  /**
   * @brief Get the number of Logical Cores
   *
   * @return int
   */
  int GetLogicalCores() { return num_logical_cores_; }

  /**
   * @brief Get the number of Physical Cores
   *
   * @return int
   */
  int GetPhysicalCores() { return num_physical_cores_; }

  /**
   * @brief Get the number of Sockets
   *
   * @return int
   */
  int GetNumSockets() { return num_sockets_; }

  /**
   * @brief Get the Assignation map
   *
   * @return const CPUAssignment&
   */
  const CPUAssignment &GetAssignation() { return topology_; }

  /**
   * @brief Destroy the CPUInfo object
   */
  virtual ~CPUInfo() = default;

 private:
  /** Number of logical cores in the system */
  int num_logical_cores_;
  /** Number of physical cores in the system */
  int num_physical_cores_;
  /** Number of sockets in the system */
  int num_sockets_;
  /** Topology and assignation within the sockets */
  CPUAssignment topology_;

  /**
   * @brief Inserts the elements within the assignation map
   *
   * @param logical_id logical core id
   * @param socket_id socket id
   * @param core_id core id
   */
  void InsertMap(const int logical_id, const int socket_id, const int core_id);
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_PROC_CPUINFO_HPP_
