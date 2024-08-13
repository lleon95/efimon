/**
 * @file macro-handling.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Defines the booleans for macro handling
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef SRC_TOOLS_MACRO_HANDLING_HPP_
#define SRC_TOOLS_MACRO_HANDLING_HPP_

#include <memory>

#ifdef ENABLE_IPMI
static constexpr bool kEnableIpmi = true;
#else
static constexpr bool kEnableIpmi = false;
#endif

#ifdef ENABLE_PERF
static constexpr bool kEnablePerf = true;
#else
static constexpr bool kEnablePerf = false;
#endif

#ifdef ENABLE_RAPL
static constexpr bool kEnableRapl = true;
#else
static constexpr bool kEnableRapl = false;
#endif

template <class T, bool enabled, typename... Args>
std::shared_ptr<T> CreateIfEnabled(Args... args) {
  if (enabled)
    return std::make_shared<T>(args...);
  else
    return nullptr;
}

template <class T, bool enabled, class I>
T* GetReadingsIfEnabled(std::shared_ptr<I> instance, const int index) {
  if (enabled) {
    return dynamic_cast<T*>(instance->GetReadings().at(index));
  } else {
    return nullptr;
  }
}

#endif  // SRC_TOOLS_MACRO_HANDLING_HPP_
