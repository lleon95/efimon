/**
 * @file proc-lister.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Class to list the processes and follow up existing processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PROC_LISTER_HPP_
#define INCLUDE_EFIMON_PROC_LISTER_HPP_

#include <string>
#include <vector>

#include <efimon/status.hpp>

namespace efimon {

/**
 * @brief Lists the processes from the system
 *
 * The lister manages three lists: lastly detected, new and dead. The last
 * three lists are COR (clear on read)
 *
 * This class helps the monitors to refresh the tracking tools and monitor
 * all the states.
 */
class ProcessLister {
 public:
  /**
   * @brief Process struct of the lister
   *
   * This summarises some information about the user
   */
  struct Process {
    /** Process ID */
    int pid;
    /** Name of the owner */
    std::string owner;
    /** Command line in string*/
    std::string cmd;
  };

  /**
   * @brief Get the Last processes
   *
   * @return std::vector<Process> with the last processes detected
   */
  std::vector<Process> GetLast() noexcept { return last_; }

  /**
   * @brief Get the processes which does not appear anymore
   *
   * Invoking this function clears the vector
   *
   * @return std::vector<Process> with the dead processes detected
   */
  std::vector<Process> GetDead() noexcept { return dead_; }

  /**
   * @brief Get the processes which newly appeared
   *
   * Invoking this function clears the vector
   *
   * @return std::vector<Process> with the new processes detected
   */
  std::vector<Process> GetNew() noexcept { return new_; }

  /**
   * @brief Refresh the lists
   *
   * @return Status of the transaction
   */
  virtual Status Detect() = 0;

  /**
   * @brief Destroy the Process Lister object
   */
  virtual ~ProcessLister() = default;

 protected:
  /** Lastly detected */
  std::vector<Process> last_;

  /** Dead */
  std::vector<Process> dead_;

  /** New */
  std::vector<Process> new_;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PROC_LISTER_HPP_ */
