/**
 * @file annotate.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer to query the perf annotate command. This depends on the
 * perf record class
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PERF_ANNOTATE_HPP_
#define INCLUDE_EFIMON_PERF_ANNOTATE_HPP_

#include <efimon/asm-classifier.hpp>
#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/perf/record.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/instruction-readings.hpp>
#include <efimon/status.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <third-party/pstream.hpp>
#include <vector>

namespace efimon {

/**
 * @brief Observer class that executes and queries the perf annotate command
 *
 * Gets information about the assembly instructions executed by a process
 * after a certain consumption threshold (usually above 0.1%). This works
 * as a sort of head for PerfRecordObserver.
 */
class PerfAnnotateObserver : public Observer {
 public:
  PerfAnnotateObserver() = delete;

  /**
   * @brief Construct a new perf annotate observer
   *
   * @param record PerfRecordObserver for executing the recording
   */
  explicit PerfAnnotateObserver(PerfRecordObserver& record);  // NOLINT

  /**
   * @brief Manually triggers the measurement in case that there is no interval
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
   * @return std::vector<Readings> vector of readings from the observer.
   * The order will be 0: InstructionReadings
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure
   *
   * Selects the CPU core affinity. Not implemented
   *
   * @param device device enumeration
   * @return Status of the transaction
   */
  Status SelectDevice(const uint device) override;

  /**
   * @brief Set the Scope of the Observer instance
   *
   * This is not implemented for this class and its use won't affect the
   * overall behaviour of the instance.
   *
   * @param scope instance scope, if it is process-specific or system-wide
   * @return Status of the transaction
   */
  Status SetScope(const ObserverScope scope) override;

  /**
   * @brief Set the process PID in case that the scope is
   * ObserverScope::PROCESS
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
   * @return process ID. Not valid
   */
  uint GetPID() const noexcept override;

  /**
   * @brief Get the Capabilities of the Observer instance
   *
   * @return vector of capabilities
   */
  const std::vector<ObserverCapabilities>& GetCapabilities() const
      noexcept override;

  /**
   * @brief Get the Status of the Observer
   *
   * @return Status of the instance
   */
  Status GetStatus() override;

  /**
   * @brief Set the Interval in milliseconds
   *
   * This does not affect the interval. Please, interact with the
   * PerfRecordObserver.
   *
   * @param interval time in milliseconds
   * @return Status of the setting process
   */
  Status SetInterval(const uint64_t interval) override;

  /**
   * @brief Clear the interval
   *
   * This does not affect the interval. Please, interact with the
   * PerfRecordObserver
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
   * @brief Destroy the Proc MemInfo Observer object
   */
  virtual ~PerfAnnotateObserver();

 private:
  /** PerfRecordObserver wrapped in this class */
  PerfRecordObserver& record_;
  /** Instruction readings: where the results are going to be encapsulated */
  InstructionReadings readings_;
  /** The results are valid */
  bool valid_;
  /** Command prefix to execute the annotation */
  std::string command_prefix_;
  /** Command suffix to execute the annotation */
  std::string command_suffix_;
  /** Classifier to construct the proper histograms */
  std::unique_ptr<AsmClassifier> classifier_;

  /** Parses the annotation results */
  Status ParseResults(redi::ipstream& ip);

  /** Reconstructs the path if it changes in the record instance */
  void ReconstructPath();
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_PERF_ANNOTATE_HPP_
