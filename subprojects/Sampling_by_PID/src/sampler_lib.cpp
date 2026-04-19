/**
 * @file sampler_lib.cpp
 * @author Diego Avila <diego.avila@uned.cr>
 *         Anthony Montero <anthonymr2010@estudiantec.cr>
 * @brief CPU sampling library implementation using eBPF
 *
 * @copyright Copyright (c) 2026. See License for Licensing
 */

#include "sampler_lib.hpp"  // NOLINT

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <cstring>
#include <string>
#include <vector>

#include "../include/prog.skel.h"

namespace cpu_sampler {

static volatile sig_atomic_t g_stop = 0;

static void sig_handler(int) { g_stop = 1; }

void request_stop() { g_stop = 1; }

static int perf_event_open(struct perf_event_attr* attr, pid_t pid, int cpu,
                           int group_fd, uint64_t flags) {
  return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

struct callback_ctx_t {
  std::vector<Sample>* out;
};

static int handle_event(void* ctx, void* data, size_t size) {
  if (size != sizeof(Sample) || !ctx || !data) {
    return 0;
  }

  auto* cb = static_cast<callback_ctx_t*>(ctx);
  cb->out->push_back(*static_cast<Sample*>(data));
  return 0;
}

bool run_sampling(const Config& config, std::vector<Sample>& out_samples,
                  std::string& error_message) {
  error_message.clear();

  if (config.target_pid <= 0) {
    error_message = "invalid target_pid";
    return false;
  }
  if (config.frequency_hz == 0) {
    error_message = "frequency_hz must be > 0";
    return false;
  }
  if (config.duration_seconds <= 0) {
    error_message = "duration_seconds must be > 0";
    return false;
  }

  g_stop = 0;
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  prog_bpf* skel = prog_bpf__open_and_load();
  if (!skel) {
    error_message = "failed to open/load BPF skeleton";
    return false;
  }

  struct perf_event_attr attr {};
  memset(&attr, 0, sizeof(attr));
  attr.type = PERF_TYPE_HARDWARE;
  attr.config = PERF_COUNT_HW_CPU_CYCLES;
  attr.size = sizeof(attr);
  attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME;
  attr.freq = 1;
  attr.sample_freq = config.frequency_hz;
  attr.precise_ip = 2;
  attr.disabled = 0;

  int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
  if (ncpus <= 0) {
    ncpus = 1;
  }

  std::vector<int> pfd(ncpus, -1);
  int prog_fd = bpf_program__fd(skel->progs.on_sample);
  bool attached = false;
  callback_ctx_t cb_ctx{&out_samples};
  struct ring_buffer* rb = nullptr;

  for (int cpu = 0; cpu < ncpus; cpu++) {
    pfd[cpu] = perf_event_open(&attr, config.target_pid, cpu, -1, 0);
    if (pfd[cpu] < 0) {
      continue;
    }

    if (ioctl(pfd[cpu], PERF_EVENT_IOC_SET_BPF, prog_fd) < 0) {
      error_message = "PERF_EVENT_IOC_SET_BPF failed";
      goto cleanup;
    }

    if (ioctl(pfd[cpu], PERF_EVENT_IOC_ENABLE, 0) < 0) {
      error_message = "PERF_EVENT_IOC_ENABLE failed";
      goto cleanup;
    }

    attached = true;
  }

  if (!attached) {
    error_message = "failed to open any perf event for target pid";
    goto cleanup;
  }

  rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_event, &cb_ctx,
                        nullptr);
  if (!rb) {
    error_message = "failed to create ring buffer";
    goto cleanup;
  }

  {
    auto start = std::chrono::steady_clock::now();
    while (!g_stop) {
      ring_buffer__poll(rb, 100);
      auto now = std::chrono::steady_clock::now();
      auto elapsed =
          std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
      if (elapsed >= config.duration_seconds) {
        break;
      }
    }
  }

  ring_buffer__free(rb);
  prog_bpf__destroy(skel);
  for (int fd : pfd) {
    if (fd >= 0) {
      close(fd);
    }
  }
  return true;

cleanup:
  if (rb) {
    ring_buffer__free(rb);
  }
  prog_bpf__destroy(skel);
  for (int fd : pfd) {
    if (fd >= 0) {
      close(fd);
    }
  }
  return false;
}

}  // namespace cpu_sampler
