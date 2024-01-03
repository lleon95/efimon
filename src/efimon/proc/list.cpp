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
#include <pwd.h>

namespace efimon {

Status ProcPsProcessLister::Detect() {
  /* Capture the last processes */
  PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS);
  proc_t proc_info;
  std::vector<ProcessLister::Process> detected;

  memset(&proc_info, 0, sizeof(proc_info));
  while (readproc(proc, &proc_info) != NULL) {
    struct passwd* pws = getpwuid(proc_info.euid);  // NOLINT
    ProcessLister::Process elem;
    elem.pid = proc_info.tid;
    elem.cmd = proc_info.cmd;
    elem.owner = pws->pw_name;
    detected.emplace_back(elem);
  }

  /* Analyse which processes are already contained in the dead and new */
  this->new_.clear();
  this->dead_.clear();

  for (auto& proc : detected) {
    auto find_crit = [&](ProcessLister::Process p1) {
      return p1.pid == proc.pid;
    };
    auto it = std::find_if(this->last_.begin(), this->last_.end(), find_crit);
    if (it == this->last_.end())
      this->new_.emplace_back(proc);
    else
      this->dead_.emplace_back(proc);
  }

  this->last_.clear();
  this->last_ = std::move(detected);

  closeproc(proc);
  return Status{};
}

} /* namespace efimon */
