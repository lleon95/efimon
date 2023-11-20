/**
 * @file thread-tree.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the thread decomposition of a given process based on the tasks
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PROC_THREAD_TREE_HPP_
#define INCLUDE_EFIMON_PROC_THREAD_TREE_HPP_

#include <vector>

#include <efimon/status.hpp>

namespace efimon {

/**
 * @brief Process thread tree reader
 *
 * Reads the threads deployed as tasks from a given process. In this case,
 * the process is specified by the PID when constructing the object.
 */
class ThreadTree {
 public:
  /**
   * @brief Default constructor (deleted)
   */
  ThreadTree() = delete;

  /**
   * @brief Construct a new ThreadTree object
   *
   * @param pid process id number
   */
  explicit ThreadTree(const int pid);

  /**
   * @brief Refresh the thread tree
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
   * @brief Destroy the Thread Tree object
   */
  virtual ~ThreadTree() = default;

 private:
  /** Process ID number */
  const int pid_;
  /** Process thread tree */
  std::vector<int> tree_;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PROC_THREAD_TREE_HPP_ */
