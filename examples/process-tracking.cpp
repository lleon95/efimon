/**
 * @file process-tracking.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of process detection and monitoring
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <efimon/proc/list.hpp>

int main(int argc, char **argv) {
  std::vector<std::string> users;

  /* Get the users to filter */
  if (argc > 1) {
    for (int i = 1; i < argc; ++i) users.emplace_back(argv[i]);
  }

  /* Monitor the processes */
  efimon::ProcPsProcessLister plister;
  plister.Detect();

  auto new_list = plister.GetNew();
  auto dead_list = plister.GetDead();
  auto last_list = plister.GetLast();

  std::cout << "Vector sizes:" << std::endl;
  std::cout << "\tNew: " << new_list.size() << std::endl;
  std::cout << "\tDead: " << dead_list.size() << std::endl;
  std::cout << "\tLast: " << last_list.size() << std::endl;

  std::cout << "Processes Running:" << std::endl;
  std::cout << "PID\t" << std::setw(20) << "CMD\tOwner" << std::endl;
  for (auto &ps : last_list) {
    bool found = std::find(users.begin(), users.end(), ps.owner) != users.end();
    bool all = users.size() == 0;
    if (found || all)
      std::cout << ps.pid << "\t" << std::setw(20) << ps.cmd << "\t" << ps.owner
                << std::endl;
  }

  return 0;
}
