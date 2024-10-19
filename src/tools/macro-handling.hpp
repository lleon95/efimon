/**
 * @file macro-handling.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Defines the booleans for macro handling
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef SRC_TOOLS_MACRO_HANDLING_HPP_
#define SRC_TOOLS_MACRO_HANDLING_HPP_

#include <efimon/status.hpp>
#include <memory>

/** Default delay: time to wait to launch a new measurement */
static constexpr int kDelay = 3;  // 3 seconds
/** Default frequency for perf sampling */
static constexpr uint kDefFrequency = 100;  // 100 Hz
/** Default number of samples to take */
static constexpr uint kDefaultSampleLimit = 100;  // 100 samples
/** Default output path where the logs are saved */
static constexpr char kDefaultOutputPath[] = "/tmp";
/** Default IPC port */
static constexpr uint kPort = 5550;
/** Number of log instance */
[[maybe_unused]] static uint logcounter = 0;

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

/**
 * @brief Create a new instance of something when enabled
 *
 * @tparam T class of something. Usually an Observer
 * @tparam enabled variable to control the creattion the instance or not
 * @tparam Args Arguments of the constructor of the class
 * @param args argument values
 * @return std::shared_ptr<T> Instance of the class to create
 */
template <class T, bool enabled, typename... Args>
std::shared_ptr<T> CreateIfEnabled(Args... args) {
  if (enabled)
    return std::make_shared<T>(args...);
  else
    return nullptr;
}

/**
 * @brief Get the Readings if the class is enabled
 *
 * Controls whether to get the readings if the implementation is enabled
 *
 * @tparam T Readings subclass to cast the resulting instance
 * @tparam enabled control variable
 * @tparam I Observer class to get the readings from
 * @param instance instance of the Observer class
 * @param index position in the Readings vector
 * @return T* pointer to the readings subclass
 */
template <class T, bool enabled, class I>
T* GetReadingsIfEnabled(std::shared_ptr<I> instance, const int index) {
  if (enabled) {
    return dynamic_cast<T*>(instance->GetReadings().at(index));
  } else {
    return nullptr;
  }
}

/**
 * @brief Trigger an Observer if enabled
 *
 * It allows triggering observers if they are enabled
 *
 * @tparam T Observer class
 * @param instance instance of the Observer class
 * @return efimon::Status
 */
template <class T>
efimon::Status TriggerIfEnabled(std::shared_ptr<T> instance) {
  if (instance) {
    return instance->Trigger();
  } else {
    return efimon::Status{};
  }
}

/** Execute the instruction if object is true */
#define EFM_SOFT_CHECK_AND_EXECUTE(obj, cmd) \
  if ((obj)) (cmd);

#endif  // SRC_TOOLS_MACRO_HANDLING_HPP_
