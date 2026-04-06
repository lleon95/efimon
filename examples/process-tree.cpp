/**
 * @file process-tree.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of the use of the process tree analyser
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/arg-parser.hpp>
#include <efimon/logger/macros.hpp>
#include <efimon/proc/process-tree.hpp>
#include <string>
#include <unordered_map>
#include <vector>

using namespace efimon;  // NOLINT

int main(int argc, char** argv) {
  ArgParser parser{argc, argv};

  int pid = 0;

  if (!parser.Exists("-pid")) {
    EFM_ERROR("PID not found. Please, use the -pid option");
  }

  pid = std::stoi(parser.GetOption("-pid"));

  ProcessTree tree{pid};
  tree.Refresh();

  auto tree_vector = tree.GetTree();

  EFM_INFO("Process with PID " + std::to_string(pid) + " has children:");
  for (const auto& proc : tree_vector) {
    std::cout << proc << std::endl;
  }

  return 0;
}
