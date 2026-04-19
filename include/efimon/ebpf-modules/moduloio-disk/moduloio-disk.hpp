/**
 * @file moduloio-disk.hpp
 * @author Diego Avila (diego.avila@uned.cr)
 * @brief ModuloIODisk eBPF-based I/O disk event tracer
 *
 * @copyright Copyright (c) 2026. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_EBPF_MODULES_MODULOIO_DISK_MODULOIO_DISK_HPP_
#define INCLUDE_EFIMON_EBPF_MODULES_MODULOIO_DISK_MODULOIO_DISK_HPP_

#include <atomic>
#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <cstdint>
#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/status.hpp>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT
#include <vector>

namespace efimon {

/**
 * @brief Object to hold disk I/O event readings
 */
class IOEventReadings : public Readings {
 public:
  /**
   * @brief Construct a new IOEventReadings object
   */
  IOEventReadings() = default;

  /**
   * @brief Get the type of readings
   *
   * @return Type of readings
   */
  Type GetType() const noexcept override;

  /**
   * @brief Store the readings in a CSV-compatible string format
   *
   * @return CSV string representation
   */
  std::string ToString() const override;

  /** Process ID that issues the I/O read */
  uint32_t pid = 0;
  /** File descriptor of the read */
  uint32_t fd = 0;
  /** Number of bytes read */
  uint64_t bytes_read = 0;
  /** Command name of the process */
  char comm[16]{};
};

/**
 * @brief Observer class that wraps the ModuloIODisk eBPF interface and
 * traces disk I/O events from processes
 *
 * This observer uses eBPF programs to efficiently trace disk I/O operations
 * via tracepoints, collecting information about read operations without the
 * overhead of ptrace.
 */
class ModuloIODiskObserver : public Observer {
 public:
  /**
   * @brief Constructor for the ModuloIODisk eBPF Observer
   *
   * @param pid process id to trace (0 for all processes)
   * @param scope only ObserverScope::PROCESS is valid
   * @param interval interval of how often the tracer is queried in
   * milliseconds. 0 for manual query.
   */
  ModuloIODiskObserver(const uint pid = 0,
                       const ObserverScope = ObserverScope::PROCESS,
                       const uint64_t interval = 0);

  /**
   * @brief Manually triggers the update in case that there is no interval
   *
   * @return Status of the transaction
   */
  Status Trigger() override;

  /**
   * @brief Get the Readings from the Observer
   *
   * Before reading it, the interval must be finished or the
   * Observer::Trigger() method must be invoked before calling this method
   *
   * @return std::vector<Readings*> vector of readings from the observer.
   * In this case, the Readings* can be dynamic-casted to IOEventReadings.
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure: not used
   *
   * @param device device enumeration
   * @return Status of the transaction
   */
  Status SelectDevice(const uint device) override;

  /**
   * @brief Set the Scope of the Observer instance (not implemented)
   *
   * @param scope instance scope, if it is process-specific or system-wide
   * @return Status of the transaction
   */
  Status SetScope(const ObserverScope scope) override;

  /**
   * @brief Set the process PID
   *
   * @param pid process ID (0 for all processes)
   * @return Status of the transaction
   */
  Status SetPID(const uint pid) override;

  /**
   * @brief Get the Scope of the Observer instance
   *
   * @return scope of the instance
   */
  ObserverScope GetScope() const noexcept override;

  /**
   * @brief Get the process ID in case of a process-specific instance
   *
   * @return process ID
   */
  uint GetPID() const noexcept override;

  /**
   * @brief Get the Capabilities of the Observer instance
   *
   * @return vector of capabilities
   */
  const std::vector<ObserverCapabilities>& GetCapabilities()
      const noexcept override;

  /**
   * @brief Get the Status of the Observer
   *
   * @return Status of the instance
   */
  Status GetStatus() override;

  /**
   * @brief Set the Interval in milliseconds
   *
   * Sets how often the observer will be refreshed
   *
   * @param interval time in milliseconds
   * @return Status of the setting process
   */
  Status SetInterval(const uint64_t interval) override;

  /**
   * @brief Clear the interval
   *
   * Avoids the instance to be automatically refreshed
   *
   * @return Status
   */
  Status ClearInterval() override;

  /**
   * @brief Resets the instance
   *
   * The effect is quite similar to destroy and re-construct the instance
   *
   * @return Status
   */
  Status Reset() override;

  /**
   * @brief Destroy the Observer
   */
  virtual ~ModuloIODiskObserver();

 private:
  /** I/O event readings: where the results are going to be encapsulated */
  std::vector<IOEventReadings> readings_;

  /** If true, the instance has valid measurements */
  bool valid_;
  /** Ring buffer file descriptor for receiving events */
  int ring_buffer_fd_;
  /** eBPF skeleton object (opaque pointer) */
  void* bpf_skeleton_;
  /** Number of events collected */
  uint64_t events_collected_;
  /** Threading for asynchronous execution */
  std::unique_ptr<std::thread> worker_thread_;
  /** Mutex for synchronisation and coherency */
  std::mutex worker_mutex_;
  /** Condition variable to wait for the termination */
  std::condition_variable worker_cv_;
  /** Flag variable to break the worker */
  std::atomic<bool> worker_running_;

  /**
   * @brief Initialize eBPF program and tracepoints
   */
  Status InitializeBPF();

  /**
   * @brief Cleanup eBPF resources
   */
  Status CleanupBPF();

  /**
   * @brief Poll the ring buffer for new events
   */
  Status PollRingBuffer();

  /**
   * @brief Process a single I/O event from the ring buffer
   */
  Status ProcessIOEvent(const uint32_t pid, const uint32_t fd,
                        const uint64_t bytes_read, const char* comm);

  /** Worker to poll events in an asynchronous way */
  void Worker();
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_EBPF_MODULES_MODULOIO_DISK_MODULOIO_DISK_HPP_
