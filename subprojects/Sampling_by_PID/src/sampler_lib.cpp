/**
 * @file sampler_lib.cpp
 * @author Diego Avila <diego.avila@uned.cr>
 *         Anthony Montero <anthonymr2010@estudiantec.cr>
 * @brief CPU sampling library implementation using eBPF
 *
 * @copyright Copyright (c) 2026. See License for Licensing
 */

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../include/prog.skel.h"

namespace cpu_sampler {

struct Sample {
  uint32_t pid;
  uint32_t tid;
  uint64_t ip;
  uint64_t ts;
};

struct Config {
  int target_pid;
  uint64_t frequency_hz;
  int duration_seconds;
};

static volatile sig_atomic_t g_i_stop = 0;

static void sig_handler(int) { g_i_stop = 1; }

void request_stop() { g_i_stop = 1; }

static int perf_event_open(struct perf_event_attr* attr, pid_t pid, int cpu,
                           int group_fd, uint64_t flags) {
  return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

struct callback_ctx_t {
  std::vector<Sample>* p_out;
};

static int handle_event(void* ctx, void* data, size_t size) {
  if (size != sizeof(Sample) || !ctx || !data) {
    return 0;
  }

  auto* p_cb = static_cast<callback_ctx_t*>(ctx);
  p_cb->p_out->push_back(*static_cast<Sample*>(data));
  return 0;
}

bool run_sampling(const Config& st_config, std::vector<Sample>& v_out_samples,
                  std::string& str_error_message) {
  str_error_message.clear();

  if (st_config.target_pid <= 0) {
    str_error_message = "invalid target_pid";
    return false;
  }
  if (st_config.frequency_hz == 0) {
    str_error_message = "frequency_hz must be > 0";
    return false;
  }
  if (st_config.duration_seconds <= 0) {
    str_error_message = "duration_seconds must be > 0";
    return false;
  }

  g_i_stop = 0;
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  prog_bpf* p_skel = prog_bpf__open_and_load();
  if (!p_skel) {
    str_error_message = "failed to open/load BPF skeleton";
    return false;
  }

  struct perf_event_attr st_attr {};
  memset(&st_attr, 0, sizeof(st_attr));
  st_attr.type = PERF_TYPE_HARDWARE;
  st_attr.config = PERF_COUNT_HW_CPU_CYCLES;
  st_attr.size = sizeof(st_attr);
  st_attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME;
  st_attr.freq = 1;
  st_attr.sample_freq = st_config.frequency_hz;
  st_attr.precise_ip = 2;
  st_attr.disabled = 0;

  int i_cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  if (i_cpu_count <= 0) {
    i_cpu_count = 1;
  }

  std::vector<int> v_perf_fds(i_cpu_count, -1);
  int i_prog_fd = bpf_program__fd(p_skel->progs.on_sample);
  bool b_attached = false;
  callback_ctx_t st_cb_ctx{&v_out_samples};
  struct ring_buffer* p_rb = nullptr;

  for (int i_cpu = 0; i_cpu < i_cpu_count; i_cpu++) {
    v_perf_fds[i_cpu] =
        perf_event_open(&st_attr, st_config.target_pid, i_cpu, -1, 0);
    if (v_perf_fds[i_cpu] < 0) {
      continue;
    }

    if (ioctl(v_perf_fds[i_cpu], PERF_EVENT_IOC_SET_BPF, i_prog_fd) < 0) {
      str_error_message = "PERF_EVENT_IOC_SET_BPF failed";
      goto cleanup;
    }

    if (ioctl(v_perf_fds[i_cpu], PERF_EVENT_IOC_ENABLE, 0) < 0) {
      str_error_message = "PERF_EVENT_IOC_ENABLE failed";
      goto cleanup;
    }

    b_attached = true;
  }

  if (!b_attached) {
    str_error_message = "failed to open any perf event for target pid";
    goto cleanup;
  }

  p_rb = ring_buffer__new(bpf_map__fd(p_skel->maps.rb), handle_event,
                          &st_cb_ctx, nullptr);
  if (!p_rb) {
    str_error_message = "failed to create ring buffer";
    goto cleanup;
  }

  {
    auto tp_start = std::chrono::steady_clock::now();
    while (!g_i_stop) {
      ring_buffer__poll(p_rb, 100);
      auto tp_now = std::chrono::steady_clock::now();
      auto i_elapsed_seconds =
          std::chrono::duration_cast<std::chrono::seconds>(tp_now - tp_start)
              .count();
      if (i_elapsed_seconds >= st_config.duration_seconds) {
        break;
      }
    }
  }

  ring_buffer__free(p_rb);
  prog_bpf__destroy(p_skel);
  for (int i_fd : v_perf_fds) {
    if (i_fd >= 0) {
      close(i_fd);
    }
  }
  return true;

cleanup:
  if (p_rb) {
    ring_buffer__free(p_rb);
  }
  prog_bpf__destroy(p_skel);
  for (int i_fd : v_perf_fds) {
    if (i_fd >= 0) {
      close(i_fd);
    }
  }
  return false;
}

}  // namespace cpu_sampler
