/**
 * @file list.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Class to list the processes and follow up existing processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PROC_LIST_HPP_
#define INCLUDE_EFIMON_PROC_LIST_HPP_

#include <vector>

#include <efimon/proc-lister.hpp>
#include <efimon/status.hpp>

namespace efimon {

/**
 * @brief Lists the processes from the system
 *
 * Implements the ProcessLister using the libprocps
 */
class ProcPsProcessLister : public ProcessLister {
 public:
  /**
   * @brief Refresh the lists
   *
   * @return Status of the transaction
   */
  Status Detect() override;

  /**
   * @brief Destroy the Process Lister object
   */
  virtual ~ProcPsProcessLister() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PROC_LIST_HPP_ */
