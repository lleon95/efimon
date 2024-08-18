/**
 * @file efimon-worker.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Defines the process daemon attachable worker
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef SRC_TOOLS_EFIMON_DAEMON_EFIMON_WORKER_HPP_
#define SRC_TOOLS_EFIMON_DAEMON_EFIMON_WORKER_HPP_

#include <atomic>
#include <efimon/logger/macros.hpp>
#include <efimon/status.hpp>
#include <memory>
#include <string>
#include <thread>  // NOLINT
#include <utility>

namespace efimon {
class EfimonWorker {
 public:
  EfimonWorker() : name_{}, pid_{0}, running_{false} {};
  explicit EfimonWorker(EfimonWorker &&worker)
      : name_{std::move(worker.name_)},
        pid_{std::move(worker.pid_)},
        running_{false},
        thread_{nullptr} {
    this->running_.store(worker.running_.load());
    this->thread_.swap(worker.thread_);
  }
  EfimonWorker(const std::string &name, const uint pid)
      : name_{name}, pid_{pid}, running_{false}, thread_{nullptr} {}
  Status Start(const uint delay) {
    if (0 == this->pid_) {
      EFM_ERROR_STATUS(
          "Invalid instance of the worker. Are you using default constructor?",
          Status::CANNOT_OPEN);
    }
    EFM_INFO("Process Monitor Start for PID: " + std::to_string(this->pid_) +
             " with delay: " + std::to_string(delay));
    return Status{};
  }
  Status Stop() {
    if (0 == this->pid_) {
      EFM_ERROR_STATUS(
          "Invalid instance of the worker. Are you using default constructor?",
          Status::CANNOT_OPEN);
    }
    EFM_INFO("Process Monitor Stopped for PID: " + std::to_string(this->pid_));
    return Status{};
  }
  virtual ~EfimonWorker() = default;

 private:
  std::string name_;
  uint pid_;
  std::atomic<bool> running_;
  std::unique_ptr<std::thread> thread_;

  Status RefreshProcStat();
};
}  // namespace efimon

#endif  // SRC_TOOLS_EFIMON_DAEMON_EFIMON_WORKER_HPP_
