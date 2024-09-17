/**
 * @file macros.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Logger macros
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_LOGGER_MACROS_HPP_
#define INCLUDE_EFIMON_LOGGER_MACROS_HPP_

#include <efimon/logger.hpp>
#include <iostream>
#include <memory>

#define EFM_INFO(msg) \
  { std::cerr << "[INFO]: " << msg << std::endl; }
#define EFM_WARN(msg) \
  { std::cerr << "[WARNING]: " << msg << std::endl; }
#define EFM_WARN_AND_BREAK(msg)                     \
  {                                                 \
    std::cerr << "[WARNING]: " << msg << std::endl; \
    break;                                          \
  }
#define EFM_ERROR(msg)                            \
  {                                               \
    std::cerr << "[ERROR]: " << msg << std::endl; \
    return -1;                                    \
  }
#define EFM_ERROR_STATUS(msg, code)               \
  {                                               \
    std::cerr << "[ERROR]: " << msg << std::endl; \
    return Status{code, msg};                     \
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

#endif /* INCLUDE_EFIMON_LOGGER_MACROS_HPP_ */
