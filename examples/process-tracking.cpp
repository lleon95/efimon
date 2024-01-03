/**
 * @file process-tracking.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of process detection and monitoring
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

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

  efimon::ProcPsProcessLister plister;
  for (int i = 0; i < 30; ++i) {
    /* Monitor the processes */
    plister.Detect();

    auto new_list = plister.GetNew();
    auto dead_list = plister.GetDead();
    auto last_list = plister.GetLast();

    if (dead_list.size() > 0) {
      std::cout << "Processes Dead:" << std::endl;
      std::cout << "PID" << std::setw(30) << "CMD" << std::setw(10) << "Owner"
                << std::endl;
      for (auto &ps : dead_list) {
        bool found =
            std::find(users.begin(), users.end(), ps.owner) != users.end();
        bool all = users.size() == 0;
        if (found || all)
          std::cout << ps.pid << std::setw(30) << ps.cmd << std::setw(10)
                    << ps.owner << std::endl;
      }
    }
    if (new_list.size() > 0) {
      std::cout << "Processes New:" << std::endl;
      std::cout << "PID" << std::setw(30) << "CMD" << std::setw(10) << "Owner"
                << std::endl;
      for (auto &ps : new_list) {
        bool found =
            std::find(users.begin(), users.end(), ps.owner) != users.end();
        bool all = users.size() == 0;
        if (found || all)
          std::cout << ps.pid << std::setw(30) << ps.cmd << std::setw(10)
                    << ps.owner << std::endl;
      }
    }
    sleep(1);
  }

  return 0;
}
