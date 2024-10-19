/**
 * @file macros.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Logger macros
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_LOGGER_MACROS_HPP_
#define INCLUDE_EFIMON_LOGGER_MACROS_HPP_

#include <ctime>
#include <efimon/logger.hpp>
#include <iostream>
#include <iterator>
#include <locale>
#include <memory>
#include <string>

/**
 * @brief Get the UTC timestamp
 *
 * @return std::string with the UTC timestamp in yyyy-mm-ddThh:mm:ssZ format
 */
[[maybe_unused]] inline std::string GetUTCTimestamp() {
  // Example of the very popular RFC 3339 format UTC time
  std::time_t time = std::time({});
  char time_string[std::size("yyyy-mm-ddThh:mm:ssZ")];  // NOLINT
  std::strftime(std::data(time_string), std::size(time_string), "%FT%TZ",
                std::gmtime(&time));
  std::string str = time_string;
  return str;
}

/**
 * @brief Information level logging
 * @param msg message to print
 */
#define EFM_INFO(msg) \
  { std::cerr << GetUTCTimestamp() << " [INFO]: " << msg << std::endl; }

/**
 * @brief Debug level logging
 * @param d debug condition (if true, the message is printed)
 * @param msg message to print
 */
#define EFM_DEBUG(d, msg)                                                 \
  {                                                                       \
    if ((d))                                                              \
      std::cerr << GetUTCTimestamp() << " [DEBUG]: " << msg << std::endl; \
  }

/**
 * @brief Warning level logging
 *
 * It just prints the message without taking any action
 *
 * @param msg message to print
 */
#define EFM_WARN(msg) \
  { std::cerr << GetUTCTimestamp() << " [WARNING]: " << msg << std::endl; }

/**
 * @brief Warning level logging
 *
 * Prints the message and submits a break to a loop
 *
 * @param msg message to print
 */
#define EFM_WARN_AND_BREAK(msg)                                           \
  {                                                                       \
    std::cerr << GetUTCTimestamp() << " [WARNING]: " << msg << std::endl; \
    break;                                                                \
  }

/**
 * @brief Error level logging
 *
 * It prints the error message and submit a return -1
 *
 * @param msg message to print
 */
#define EFM_ERROR(msg)                                                  \
  {                                                                     \
    std::cerr << GetUTCTimestamp() << " [ERROR]: " << msg << std::endl; \
    return -1;                                                          \
  }

/**
 * @brief Error level logging
 *
 * It prints the error message and return a Status object
 *
 * @param msg message to print
 * @param code error code
 */
#define EFM_ERROR_STATUS(msg, code)                                     \
  {                                                                     \
    std::cerr << GetUTCTimestamp() << " [ERROR]: " << msg << std::endl; \
    return Status{code, msg};                                           \
  }

/**
 * @brief Check a Status object and execute a command
 *
 * It checks the status object of inst and executes func
 * In case that the code is an error
 *
 * @param inst instance of Status to check
 * @param func function to execute based on the message.
 */
#define EFM_CHECK(inst, func)                \
  {                                          \
    Status s_ = (inst);                      \
    if (s_.code != Status::OK) func(s_.msg); \
  }

/**
 * @brief Check a Status object and perform an error
 *
 * It checks the status object of inst and executes EFM_ERROR
 * in case of an error
 *
 * @param inst instance of Status to check
 */
#define EFM_CRITICAL_CHECK(inst)                  \
  {                                               \
    Status s_ = (inst);                           \
    if (s_.code != Status::OK) EFM_ERROR(s_.msg); \
  }

/**
 * @brief Check a Status object and return the status in case of error
 *
 * It checks the status object of inst and returns it back in case
 * of containing an error
 *
 * @param inst instance of Status to check
 */
#define EFM_CHECK_STATUS(inst)            \
  {                                       \
    Status s_ = (inst);                   \
    if (s_.code != Status::OK) return s_; \
  }

#define EFM_RES std::cout

/**
 * @brief Log a value into a logger instance
 *
 * It takes the variables of a single log line and assigns a value to a single
 * variable by name.
 *
 * @param vars variables of the logging instance
 * @param name name of the variable to log
 * @param val value corresponding to the variable
 */
#define LOG_VAL(vars, name, val)   \
  (vars)[name] = std::make_shared< \
      Logger::Value<std::remove_reference<decltype(val)>::type>>((val));

#endif  // INCLUDE_EFIMON_LOGGER_MACROS_HPP_
