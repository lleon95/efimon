/**
 * @file sampling-by-pid.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Sampling-by-PID eBPF-based CPU cycle sampler
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_EBPF_MODULES_SAMPLING_BY_PID_SAMPLING_BY_PID_HPP_
#define INCLUDE_EFIMON_EBPF_MODULES_SAMPLING_BY_PID_SAMPLING_BY_PID_HPP_

#include <atomic>
#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <cstdint>
#include <efimon/asm-classifier.hpp>
#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/instruction-readings.hpp>
#include <efimon/status.hpp>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT
#include <vector>

namespace efimon {

/**
 * @brief Observer class that wraps the eBPF Sampling-by-PID interface and
 * collects CPU cycle samples from a running process
 *
 * This observer uses eBPF programs to efficiently sample CPU cycles via
 * perf events, collecting instruction pointers without the overhead of
 * ptrace. The samples are decoded using the AsmClassifier.
 */
class SamplingByPIDObserver : public Observer {
 public:
  /**
   * @brief Constructor for the Sampling-by-PID eBPF Observer
   *
   * @param pid process id to attach
   * @param scope only ObserverScope::PROCESS is valid
   * @param interval interval of how often the profiler is queried in
   * milliseconds. 0 for manual query.
   * @param frequency sampling frequency in Hz (how often CPU cycles are
   * sampled). Default 50000 Hz.
   */
  SamplingByPIDObserver(const uint pid = 0,
                        const ObserverScope = ObserverScope::PROCESS,
                        const uint64_t interval = 0,
                        const uint64_t frequency = 50000);

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
   * In this case, the Readings* can be dynamic-casted to:
   *
   * The order will be 0: InstructionReadings
   *
   * More to be defined during the way
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
   * @param pid process ID
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
  virtual ~SamplingByPIDObserver();

 private:
  /** Instruction readings: where the results are going to be encapsulated */
  InstructionReadings readings_;

  uint64_t frequency_;
  /** If true, the instance has valid measurements */
  bool valid_;
  /** Classifier to construct the proper histograms */
  std::unique_ptr<AsmClassifier> classifier_;
  /** Ring buffer file descriptor for receiving samples */
  int ring_buffer_fd_;
  /** eBPF skeleton object (opaque pointer) */
  void* bpf_skeleton_;
  /** Vector of perf event file descriptors */
  std::vector<int> perf_fds_;
  /** Instruction pointer from current sample */
  uint64_t current_ip_;
  /** Process ID from current sample */
  uint32_t current_sample_tid_;
  /** Timestamp from current sample */
  uint64_t current_sample_ts_;
  /** Memory contents from the instruction at the given IP */
  uint8_t inst_mem_[16];
  /** Instruction string */
  std::string inst_;
  /** Number of samples collected */
  uint64_t samples_collected_;
  /** Threading for asynchronous execution */
  std::unique_ptr<std::thread> worker_thread_;
  /** Mutex for synchronisation and coherency */
  std::mutex worker_mutex_;
  /** Condition variable to wait for the termination */
  std::condition_variable worker_cv_;
  /** Flag variable to break the worker */
  std::atomic<bool> worker_running_;

  /**
   * @brief Initialize eBPF program and perf events
   */
  Status InitializeBPF();

  /**
   * @brief Cleanup eBPF resources
   */
  Status CleanupBPF();

  /**
   * @brief Police the ring buffer for new samples
   */
  Status PollRingBuffer();

  /**
   * @brief Process a single sample from the ring buffer
   */
  Status ProcessSample(const uint32_t pid, const uint32_t tid,
                       const uint64_t ip, const uint64_t ts);

  /**
   * @brief Decode the instruction at the given IP
   */
  Status DecodeInstruction(const uint64_t ip);

  /** Parses the annotation results */
  Status ParseResults();

  /** Normalise the annotation results */
  Status NormaliseResults();

  /** Worker to poll samples in an asynchronous way */
  void Worker();
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_EBPF_MODULES_SAMPLING_BY_PID_SAMPLING_BY_PID_HPP_
