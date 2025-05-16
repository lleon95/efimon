/**
 * @file list.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Class to list the processes and follow up existing processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <libproc2/pids.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <efimon/proc/list.hpp>
#include <efimon/status.hpp>
#include <utility>
#include <vector>

namespace efimon {
Status ProcPsProcessLister::Detect() {
  /* Capture the last processes */
  struct pids_info *info = nullptr;
  struct pids_fetch *fetch = nullptr;
  enum pids_item items[] = {PIDS_ID_PID, PIDS_CMD, PIDS_ID_EUSER};
  // constexpr int numitems = 3 ;

  // Optional: hide kernel threads
  setenv("LIBPROC_HIDE_KERNEL", "1", 1);
  procps_pids_new(&info, items, std::size(items));
  fetch = procps_pids_reap(info, PIDS_FETCH_TASKS_ONLY);

  std::vector<ProcessLister::Process> detected;

  for (int i = 0; i < fetch->counts->total; i++) {
    ProcessLister::Process elem;
    struct pids_stack *stack = fetch->stacks[i];
    elem.pid = PIDS_VAL(0, u_int, stack, info);
    elem.cmd = PIDS_VAL(1, str, stack, info);
    elem.owner = PIDS_VAL(2, str, stack, info);
    detected.emplace_back(elem);
  }

  /* Analyse which processes are already contained in the dead and new */
  this->new_.clear();
  this->dead_.clear();

  for (auto &proc : detected) {
    auto find_crit = [&](ProcessLister::Process p1) {
      return p1.pid == proc.pid;
    };
    auto it = std::find_if(this->last_.begin(), this->last_.end(), find_crit);
    if (it == this->last_.end()) this->new_.emplace_back(proc);
  }

  for (auto &proc : this->last_) {
    auto find_crit = [&](ProcessLister::Process p1) {
      return p1.pid == proc.pid;
    };
    auto it = std::find_if(detected.begin(), detected.end(), find_crit);
    if (it == detected.end()) this->dead_.emplace_back(proc);
  }

  this->last_.clear();
  this->last_ = std::move(detected);

  procps_pids_unref(&info);
  return Status{};
}

} /* namespace efimon */
