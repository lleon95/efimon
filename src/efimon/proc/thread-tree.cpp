/**
 * @file thread-tree.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the thread decomposition of a given process based on the tasks
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <efimon/proc/thread-tree.hpp>

namespace efimon {

ThreadTree::ThreadTree(const int pid) : pid_{pid}, tree_{} {
  Status ret = this->Refresh();
  if (Status::OK != ret.code) {
    throw ret;
  }
}

Status ThreadTree::Refresh() {
  Status ret{};

  return ret;
}

const std::vector<int> &ThreadTree::GetTree() const noexcept { return tree_; }

} /* namespace efimon */
