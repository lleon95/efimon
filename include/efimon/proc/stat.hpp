/**
 * @file stat.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Observer to query the /proc/pid/stat
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PROC_STAT_HPP_
#define INCLUDE_EFIMON_PROC_STAT_HPP_

#include <vector>

#include <efimon/observer-enums.hpp>
#include <efimon/observer.hpp>
#include <efimon/readings.hpp>
#include <efimon/readings/cpu-readings.hpp>
#include <efimon/readings/ram-readings.hpp>
#include <efimon/status.hpp>

#define MAX_NUM_CPUS 1024

namespace efimon {

/**
 * @brief Observer class that queries /proc/pid/stat
 *
 * This measures the RAM usage, CPU usage and uptime
 */
class ProcStatObserver : public Observer {
 public:
  ProcStatObserver() = delete;

  /**
   * @brief Payload structure extracted from /proc/pid/stat
   *
   */
  typedef struct {
    /** Process ID */
    int pid;
    /** Process State. Refer to ProcState for the values */
    char state;
    /** Amount of time that this process has been scheduled in user mode,
    measured in clock ticks */
    uint64_t utime;
    /** Amount of time that this process has been scheduled in kernel mode,
    measured in clock ticks */
    uint64_t stime;
    /** Amount of time that this process has been waiting scheduled in
    user mode, measured in clock ticks */
    int64_t cutime;
    /** Amount of time that this process has been waiting scheduled in
    kernel mode, measured in clock ticks */
    int64_t cstime;
    /** The time the process started after system boot. Measured in clock
    ticks */
    uint64_t starttime;
    /** Total virtual memory size in bytes. */
    uint64_t vsize;
    /** Resident Set Size: number of pages the process has in real memory.
    This is just the pages which count toward text, data, or stack space.
    This does not include pages which have not been demand-loaded in, or which
    are swapped out */
    int64_t rss;
    /** Index of the processor where it is running */
    int processor;
    /** Foreign: total time spent by the process */
    uint64_t total;
    /** Foreign: active time of the process*/
    uint64_t active;
  } ProcStatData;

  /**
   * @brief Payload structure extracted from /proc/stat
   *
   * This intends to hold the information for a single CPU core. The final must
   * be an array of structs.
   */
  typedef struct {
    /** Time spent in user mode. */
    uint64_t user;
    /** Time spent in user mode with low priority (nice) */
    uint64_t nice;
    /** Time spent in system mode. */
    uint64_t system;
    /** Time spent in the idle task. USER_HZ times second value of uptime */
    uint64_t idle;
    /** Time waiting for I/O to complete */
    uint64_t iowait;
    /** Processor index */
    uint32_t cpu_idx;
    /** Foreign: active time of the process*/
    uint64_t active;
    /** Foreign: total time spent by the process */
    uint64_t total;
  } ProcStatGlobalData;

  /**
   * @brief Construct a new Proc Stat Observer
   *
   * @param pid process id (it is specific to processes)
   * @param scope if defined as ObserverScope::SYSTEM, pid is ignored
   * @param interval interval of how often the proc is queried. 0 for manual
   * query
   */
  ProcStatObserver(const uint pid, const ObserverScope scope,
                   const uint64_t interval);

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
   * The order will be 0: CPU Readings, 1: RAM Readings
   */
  std::vector<Readings*> GetReadings() override;

  /**
   * @brief Select the device to measure
   *
   * This method can be implemented or not, depending on the type of observer.
   * Some valid observers are the CPU (measuring a socket or core)
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
   * @return process ID
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
   * @brief Destroy the Proc Stat Observer object
   */
  virtual ~ProcStatObserver();

 private:
  /** Process aliveness */
  bool alive_;
  /** Proccess data payload when reading /proc/pid/stat */
  ProcStatData proc_data_;
  /** Proccess data payload when reading /proc/stat */
  ProcStatGlobalData proc_global_data_[MAX_NUM_CPUS];
  /** Readings from the CPU */
  CPUReadings cpu_readings_;
  /** Readings from the RAM */
  RAMReadings ram_readings_;
  /** Uptime */
  uint64_t uptime_;
  /** Uptime in idle processes */
  uint64_t uptime_idle_;
  /** The observer is a global system-wide observer*/
  bool global_;

  /**
   * @brief Get the system uptime
   *
   * @return uint64_t timestamp relative to the system uptime in milliseconds
   */
  uint64_t GetUptime();

  /**
   * @brief Read the /proc/pid/stat file
   *
   */
  void GetProcStat();

  /**
   * @brief Read the /proc/stat for all
   */
  void GetGlobalProcStat();

  /**
   * @brief Check if the process is still alive
   */
  void CheckAlive();

  /**
   * @brief Translate the readings from ProcStatData to CPUReadings
   */
  void TranslateReadings() noexcept;

  /**
   * @brief Translate the readings from ProcStatGlobalData to CPUReadings
   */
  void TranslateGlobalReadings() noexcept;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PROC_STAT_HPP_ */
