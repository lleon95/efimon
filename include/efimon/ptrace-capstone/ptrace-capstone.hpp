/**
 * @file ptrace-capstone.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Ptrace-capstone-based ASM low-weight extractor
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PTRACE_CAPSTONE_PTRACE_CAPSTONE_HPP_
#define INCLUDE_EFIMON_PTRACE_CAPSTONE_PTRACE_CAPSTONE_HPP_

#include <efimon/asm-classifier.hpp>
#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/instruction-readings.hpp>
#include <efimon/status.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace efimon {

/**
 * @brief Observer class that wraps the Ptrace-Capstone interface and
 * gets the assembly code from a running PID
 */
class PTraceCapstoneObserver : public Observer {
 public:
  /**
   * @brief Constructor for the Ptrace-Capstone Meter Observer
   *
   * @param pid process id to attach
   * @param scope only ObserverScope::PROCESS is valid
   * @param interval interval of how often the profiler is queried in
   * milliseconds. 0 for manual query.
   */
  PTraceCapstoneObserver(const uint pid = 0,
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
  virtual ~PTraceCapstoneObserver();

 private:
  /** Instruction readings: where the results are going to be encapsulated */
  InstructionReadings readings_;
  /** If true, the instance has valid measurements */
  bool valid_;
  /** Classifier to construct the proper histograms */
  std::unique_ptr<AsmClassifier> classifier_;
  /** Register for the PC Counter */
  uintptr_t pc_;
  /** Memory contents from the instruction at the given PC */
  uint8_t pc_inst_mem_[16];
  /** Instruction string */
  std::string inst_;
  /** Number of samples */
  uint64_t samples_;

  /**
   * @brief Get a sample from the instruction counter
   */
  Status GetSample();

  /**
   * @brief Decodes the obtained sample
   */
  Status DecodeSample();

  /** Parses the annotation results */
  Status ParseResults();

  /** Normalise the annotation results */
  Status NormaliseResults();
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_PTRACE_CAPSTONE_PTRACE_CAPSTONE_HPP_
