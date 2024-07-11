/**
 * @file nvidia.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief NVIDIA GPU analyser: consumption in RAM, usage and power (if
 * available)
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_GPU_NVIDIA_HPP_
#define INCLUDE_EFIMON_GPU_NVIDIA_HPP_

#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/gpu-readings.hpp>
#include <vector>

#ifdef ENABLE_NVIDIA_NVML
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <nvml.h>
#pragma GCC diagnostic pop
#endif  // ENABLE_NVIDIA_NVML

namespace efimon {

static constexpr int kNumProcessLimit = 256;
static constexpr int kNumMaxHandles = 32;

/**
 * @brief Observer class that wraps the NVML interface and gets the
 * consumption, usage and power in a granular and general overview.
 *
 */
class NVIDIAMeterObserver : public Observer {
 public:
  /**
   * @brief Constructor for the NVIDIA Meter Observer
   *
   * @param pid process id to attach in
   * @param scope measurements attached to a PID or system-wide
   * @param interval interval of how often the profiler is queried in
   * milliseconds. 0 for manual query.
   */
  NVIDIAMeterObserver(const uint pid,
                      const ObserverScope = ObserverScope::SYSTEM,
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
   * - GPUReadings. The metrics here corresponds to the measurements
   * from NVML and depends on the GPU technology.
   *
   * More to be defined during the way
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure.
   *
   * It corresponds to the GPU id. If not invoked, it takes into account
   * all GPUs. If the number exceeds the indexation of the GPUs,
   * it takes all
   *
   * @param device device enumeration
   * @return Status of the transaction
   */
  Status SelectDevice(const uint device) override;

  /**
   * @brief Set the Scope of the Observer instance
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
  virtual ~NVIDIAMeterObserver();

  /**
   * @brief Gets the number of GPUs
   *
   * @return uint32_t number of GPUS
   */
  static uint32_t GetGPUCount();

 private:
  /** The results are valid */
  bool valid_;

#ifdef ENABLE_NVIDIA_NVML
  /** Initialised */
  bool init_;
  /** GPU device */
  uint device_;
  /** Num devices */
  uint32_t num_devices_;
  /** Readings from GPU */
  GPUReadings readings_;
  /** Device handler from the ID: needs to be refreshed every device change */
  nvmlDevice_t device_handles_[kNumMaxHandles];
  /** Running processes */
  nvmlProcessUtilizationSample_t running_processes_[kNumProcessLimit];
  /** System usage */
  nvmlUtilization_t sys_usage_;

  /**
   * @brief Get the Process Stats
   *
   * @param device device to account the stats
   * @param pid pid of the process to get the stats from
   * @return Status
   */
  Status GetProcessStats(const uint pid, const uint device);

  /**
   * @brief Get the System Stats
   *
   * @param device device to account the stats
   * @return Status
   */
  Status GetSystemStats(const uint device);
#endif  // ENABLE_NVIDIA_NVML
};

#ifndef ENABLE_NVIDIA_NVML
inline uint32_t NVIDIAMeterObserver::GetGPUCount() { return 0; }
#endif  // ENABLE_NVIDIA_NVML

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_GPU_NVIDIA_HPP_
