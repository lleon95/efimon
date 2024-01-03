/**
 * @file list.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Class to list the processes and follow up existing processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/proc/list.hpp>
#include <efimon/status.hpp>

#include <algorithm>
#include <cstring>

#include <proc/readproc.h>

namespace efimon {

Status ProcPsProcessLister::Detect() {
  /* Capture the last processes */
  PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS);
  proc_t proc_info;
  std::vector<int> detected;

  memset(&proc_info, 0, sizeof(proc_info));
  while (readproc(proc, &proc_info) != NULL) {
    detected.emplace_back(proc_info.tid);
  }

  /* Analyse which processes are already contained in the dead and new */
  this->new_.clear();
  this->dead_.clear();

  for (int tid : detected) {
    auto it = std::find(this->last_.begin(), this->last_.end(), tid);
    if (it == this->last_.end())
      this->new_.emplace_back(tid);
    else
      this->dead_.emplace_back(tid);
  }

  this->last_.clear();
  this->last_ = std::move(detected);

  closeproc(proc);
  return Status{};
}

} /* namespace efimon */
