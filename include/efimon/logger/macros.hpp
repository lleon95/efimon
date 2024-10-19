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

[[maybe_unused]] inline std::string GetUTCTimestamp() {
  // Example of the very popular RFC 3339 format UTC time
  std::time_t time = std::time({});
  char time_string[std::size("yyyy-mm-ddThh:mm:ssZ")];  // NOLINT
  std::strftime(std::data(time_string), std::size(time_string), "%FT%TZ",
                std::gmtime(&time));
  std::string str = time_string;
  return str;
}

#define EFM_INFO(msg) \
  { std::cerr << GetUTCTimestamp() << " [INFO]: " << msg << std::endl; }
#define EFM_WARN(msg) \
  { std::cerr << GetUTCTimestamp() << " [WARNING]: " << msg << std::endl; }
#define EFM_WARN_AND_BREAK(msg)                                           \
  {                                                                       \
    std::cerr << GetUTCTimestamp() << " [WARNING]: " << msg << std::endl; \
    break;                                                                \
  }
#define EFM_ERROR(msg)                                                  \
  {                                                                     \
    std::cerr << GetUTCTimestamp() << " [ERROR]: " << msg << std::endl; \
    return -1;                                                          \
  }
#define EFM_ERROR_STATUS(msg, code)                                     \
  {                                                                     \
    std::cerr << GetUTCTimestamp() << " [ERROR]: " << msg << std::endl; \
    return Status{code, msg};                                           \
  }
#define EFM_CHECK(inst, func)                \
  {                                          \
    Status s_ = (inst);                      \
    if (s_.code != Status::OK) func(s_.msg); \
  }
#define EFM_CRITICAL_CHECK(inst)                  \
  {                                               \
    Status s_ = (inst);                           \
    if (s_.code != Status::OK) EFM_ERROR(s_.msg); \
  }
#define EFM_CHECK_STATUS(inst)            \
  {                                       \
    Status s_ = (inst);                   \
    if (s_.code != Status::OK) return s_; \
  }
#define EFM_RES std::cout
#define LOG_VAL(vars, name, val)   \
  (vars)[name] = std::make_shared< \
      Logger::Value<std::remove_reference<decltype(val)>::type>>((val));

#endif  // INCLUDE_EFIMON_LOGGER_MACROS_HPP_
