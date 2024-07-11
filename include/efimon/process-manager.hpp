/**
 * @file process-manager.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Class to launch processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_PROCESS_MANAGER_HPP_
#define INCLUDE_EFIMON_PROCESS_MANAGER_HPP_

#include <efimon/status.hpp>
#include <ostream>
#include <string>
#include <third-party/pstream.hpp>
#include <vector>

namespace efimon {

/**
 * @brief This class is capable of launching processes outside of efimon
 * and it is useful for wrapping and launching external processes to be
 * analysed.
 *
 * This is based on the pstream class
 */
class ProcessManager {
 public:
  /**
   * @brief Reports the data from the console
   */
  enum Mode {
    /** Does not report anything */
    SILENT = 0,
    /** Reports only STDOUT */
    STDOUT,
    /** Reports only STDERR */
    STDERR,
    /** Reports both */
    BOTH
  };

  /**
   * @brief Default constructor
   *
   * Constructs a process without an explicit command. To start the process,
   * please, use the ProcessManager::Open method
   */
  ProcessManager() = default;

  /**
   * @brief Custom constructor for commands
   *
   * Executes the command specified. It could include arguments within the
   * stream
   * @param cmd command to launch
   * @param mode mode of the console
   * @param stream printing stream (defaults to none, which means that
   * everything is redirected to the standard output unless SILENT mode is
   * specified)
   */
  explicit ProcessManager(const std::string &cmd, const Mode mode = BOTH,
                          std::ostream *stream = nullptr);

  /**
   * @brief Custom constructor for commands with arguments
   *
   * Executes the command specified. The arguments are passed through the args
   * @param cmd command to launch
   * @param args arguments to attach
   * @param mode mode of the console
   * @param stream printing stream (defaults to none, which means that
   * everything is redirected to the standard output unless SILENT mode is
   * specified)
   */
  ProcessManager(const std::string &cmd, const std::vector<std::string> &args,
                 const Mode mode = BOTH, std::ostream *stream = nullptr);

  /**
   * @brief Custom commands launcher
   *
   * Executes the command specified. It could include arguments within the
   * stream
   * @param cmd command to launch
   * @param mode mode of the console
   * @param stream printing stream (defaults to none, which means that
   * everything is redirected to the standard output unless SILENT mode is
   * specified)
   */
  Status Open(const std::string &cmd, const Mode mode = BOTH,
              std::ostream *stream = nullptr);

  /**
   * @brief Custom constructor for commands with arguments
   *
   * Executes the command specified. The arguments are passed through the args
   * @param cmd command to launch
   * @param args arguments to attach
   * @param mode mode of the console
   * @param stream printing stream (defaults to none, which means that
   * everything is redirected to the respective output unless SILENT mode is
   * specified)
   */
  Status Open(const std::string &cmd, const std::vector<std::string> &args,
              const Mode mode = BOTH, std::ostream *stream = nullptr);

  /**
   * @brief Gets the PID
   *
   * @return pid_t PID of the process (if running). Otherwise, it returns -1
   */
  pid_t GetPID();

  /**
   * @brief Returns the stream
   *
   * @return returns the ostream under the hood.
   */
  redi::ipstream &GetOStream();

  /**
   * @brief Synchronises the execution and waits until completion
   *
   * @param quick quick check to see if the process is still running
   * @return Runtime
   */
  Status Sync(const bool quick = false);

  /**
   * @brief Checks that the process is running
   *
   * @return true if it is running
   */
  bool IsRunning();

  /**
   * @brief Close the process
   */
  Status Close();

  /**
   * @brief Destroy the Process Lister object
   */
  virtual ~ProcessManager() = default;

 private:
  /** Process wrapper */
  redi::ipstream ip_;
  /** Mode */
  Mode mode_;
  /** Stream out */
  std::ostream *stream_ = nullptr;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_PROCESS_MANAGER_HPP_ */
