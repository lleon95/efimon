/**
 * @file uptime.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Gets the system's uptime
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mutex>  // NOLINT

#define EXPORT __attribute__((visibility("default")))

EXPORT std::mutex m_single_uptime;

namespace efimon {
uint64_t GetUptime() {
  std::scoped_lock lock(m_single_uptime);
  float uptime = 0.f;
  float uptime_idle = 0.f;
  FILE *proc_uptime_file = fopen("/proc/uptime", "r");

  if (proc_uptime_file == NULL) {
    return 0;
  }

  fscanf(proc_uptime_file, "%f %f", &uptime, &uptime_idle);
  fclose(proc_uptime_file);

  uint64_t val = static_cast<uint64_t>(uptime * sysconf(_SC_CLK_TCK)) * 10;

  return val;
}
} /* namespace efimon */
