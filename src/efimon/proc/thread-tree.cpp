/**
 * @file thread-tree.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the thread decomposition of a given process based on the tasks
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <efimon/proc/thread-tree.hpp>

#include <filesystem>
#include <string>

namespace efimon {

ThreadTree::ThreadTree(const int pid) : pid_{pid}, tree_{} {
  path_ = "/proc/" + std::to_string(pid) + "/task";
  this->Refresh();
}

Status ThreadTree::Refresh() {
  Status ret{};

  auto dir_iterator = std::filesystem::directory_iterator(path_);
  tree_.clear();

  for (const auto &entry : dir_iterator) {
    std::string tpath = entry.path();
    auto lpos = tpath.find_last_of('/');
    std::string tspid = tpath.substr(lpos + 1);
    tree_.push_back(std::stoi(tspid));
  }

  return ret;
}

const std::vector<int> &ThreadTree::GetTree() const noexcept { return tree_; }

} /* namespace efimon */
