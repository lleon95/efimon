/**
 * @file process-tree.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the process decomposition of a given process based on the
 * children processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PROC_PROCESS_TREE_HPP_
#define INCLUDE_EFIMON_PROC_PROCESS_TREE_HPP_

#include <efimon/status.hpp>
#include <string>
#include <vector>

namespace efimon {

/**
 * @brief Process tree reader
 *
 * Reads the processes deployed as tasks from a given process. In this case,
 * the parent process is specified by the PID when constructing the object.
 */
class ProcessTree {
 public:
  /**
   * @brief Default constructor (deleted)
   */
  ProcessTree() = delete;

  /**
   * @brief Construct a new ProcessTree object
   *
   * @param pid process id number
   */
  explicit ProcessTree(const int pid);

  /**
   * @brief Refresh the Process tree
   *
   * It re-reads the process tree. In case if the process does not
   * exist anymore, it returns NOT_FOUND
   *
   * @return efimon::Status It can be OK or NOT_FOUND
   */
  efimon::Status Refresh();

  /**
   * @brief Get the Tree vector
   *
   * Gets a vector with the process tree
   * @return const std::vector<int>& with the tree
   */
  const std::vector<int> &GetTree() const noexcept;

  /**
   * @brief Destroy the Process Tree object
   */
  virtual ~ProcessTree() = default;

 private:
  /** Process ID number */
  const int pid_;
  /** Process process tree */
  std::vector<int> tree_;
  /** Process tree path */
  std::string path_;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PROC_PROCESS_TREE_HPP_ */
