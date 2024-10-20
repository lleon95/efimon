/**
 * @file process-tree.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Parses the process decomposition of a given process based on the tasks
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/proc/process-tree.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace efimon {

ProcessTree::ProcessTree(const int pid) : pid_{pid}, tree_{} {
  path_ = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(pid) +
          "/children";
  this->Refresh();
}

Status ProcessTree::Refresh() {
  // Clear the tree
  tree_.clear();

  // Open the file for querying the children
  std::ifstream fs{this->path_};
  if (!fs.is_open()) {
    return Status{Status::NOT_FOUND, "Cannot access the file for children"};
  }

  // The file seems to be a single line of PIDs separated by spaces
  std::string intermediate, line;
  std::getline(fs, line);
  std::stringstream linestream(line);

  int cpid;
  tree_.push_back(this->pid_);
  while (true) {
    linestream >> cpid;
    if (linestream.eof()) break;
    tree_.push_back(cpid);
  }

  return Status{};
}

const std::vector<int> &ProcessTree::GetTree() const noexcept { return tree_; }

} /* namespace efimon */
