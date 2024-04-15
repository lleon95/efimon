/**
 * @file intel.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Power consumption wrapper for Intel Processors. In includes many
 * amenities such as CPU utilisation, Bandwidth, Power Consumption (if
 * available in the motherboard, memory hiearchy details like hits and
 * misses, and so on).
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <cstdint>
#include <efimon/power/intel.hpp>
#include <efimon/status.hpp>
#include <mutex>  // NOLINT
#include <string>
#include <third-party/pcm.hpp>
#include <vector>

namespace efimon {

extern uint64_t GetUptime();

namespace priv {
/** Mutex to guard the pcm_instance_ access */
static std::mutex pcm_mutex_;
/** PCM Instance that must be only generated once */
static pcm::PCM* pcm_instance_ = nullptr;
/** PCM socket 1 measurements are valid (0: invalid, 1: invalid, 2: valid) */
static int pcm_valid_ = 0;
/**
 * Core state variables: measurements.
 * The 1 is the prev and 2 is the current
 */
static std::vector<pcm::CoreCounterState> core_state_1_, core_state_2_;
/**
 * Socket state variables: measurements.
 * The 1 is the prev and 2 is the current
 */
static std::vector<pcm::SocketCounterState> socket_state_1_, socket_state_2_;
/**
 * System state variables: measurements.
 * The 1 is the prev and 2 is the current
 */
static pcm::SystemCounterState system_state_1_, system_state_2_;
} /* namespace priv */

IntelMeterObserver::IntelMeterObserver(const uint pid,
                                       const ObserverScope scope,
                                       const uint64_t interval)
    : Observer{}, valid_{false} {
  uint64_t type = static_cast<uint64_t>(ObserverType::CPU) |
                  static_cast<uint64_t>(ObserverType::INTERVAL);

  this->pid_ = pid;
  this->interval_ = interval;

  if (ObserverScope::SYSTEM != scope) {
    throw Status{Status::INVALID_PARAMETER, "Process-scope is not supported"};
  }

  /* Lock access to pcm_instance_ */
  priv::pcm_mutex_.lock();
  if (!priv::pcm_instance_) {
    priv::pcm_instance_ = pcm::PCM::getInstance();

    const pcm::PCM::ErrorCode status = priv::pcm_instance_->program(
        pcm::PCM::DEFAULT_EVENTS, nullptr, false, -1);

    std::string msg;
    switch (status) {
      case pcm::PCM::Success:
        break;
      case pcm::PCM::MSRAccessDenied:
        msg =
            "Access to Intel(r) Performance Counter Monitor has denied (no MSR "
            "or PCI CFG space access).\n";
        throw Status{Status::ACCESS_DENIED, msg};
      case pcm::PCM::PMUBusy:
        msg =
            "Access to Intel(r) Performance Counter Monitor has denied "
            "(Performance Monitoring Unit is occupied by other application). "
            "Try to stop the application that uses PMU.\n Alternatively you "
            "can try running PCM with option -r to reset PMU.\n";
        throw Status{Status::RESOURCE_BUSY, msg};
      default:
        msg =
            "Access to Intel(r) Performance Counter Monitor has denied "
            "(Unknown error).\n";
        throw Status{Status::ACCESS_DENIED, msg};
    }

    priv::pcm_mutex_.unlock();
    this->Reset();
    this->Trigger();
  }

  if (priv::pcm_instance_->packageEnergyMetricsAvailable()) {
    type |= static_cast<uint64_t>(ObserverType::POWER);
  }

  this->caps_.emplace_back();
  this->caps_[0].type = type;
}

Status IntelMeterObserver::Trigger() {
  /* Get the data */
  std::scoped_lock<std::mutex> lock(priv::pcm_mutex_);
  if (priv::pcm_valid_ > 0) {
    priv::pcm_instance_->getAllCounterStates(
        priv::system_state_2_, priv::socket_state_2_, priv::core_state_2_);
  } else {
    priv::pcm_instance_->getAllCounterStates(
        priv::system_state_1_, priv::socket_state_1_, priv::core_state_1_);
    priv::pcm_valid_++;
    return Status{};
  }

  priv::pcm_valid_ =
      priv::pcm_valid_ == 2 ? priv::pcm_valid_ : priv::pcm_valid_ + 1;

  /* Set readings common metadata */
  auto time = GetUptime();
  this->readings_.type = static_cast<uint64_t>(ObserverType::CPU) |
                         static_cast<uint64_t>(ObserverType::POWER);
  this->readings_.difference = time - this->readings_.timestamp;
  this->readings_.timestamp = time;

  /* Parse the readings */
  this->ParseResults();

  this->valid_ = true;

  std::swap(priv::system_state_1_, priv::system_state_2_);
  std::swap(priv::socket_state_1_, priv::socket_state_2_);
  std::swap(priv::core_state_1_, priv::core_state_2_);
  return Status{};
}

void IntelMeterObserver::ParseResults() {
  this->readings_.overall_power = 0.f;
  this->readings_.socket_power.clear();
  uint num_sockets = priv::pcm_instance_->getNumSockets();

  /* Get usage in terms of IPC */
  std::vector<uint> socket_core_count;
  this->readings_.overall_usage = 0.f;
  this->readings_.socket_usage.clear();
  this->readings_.socket_usage.resize(num_sockets, 0);
  socket_core_count.resize(num_sockets, 0);
  this->readings_.core_usage.clear();

  for (uint i = 0; i < priv::pcm_instance_->getNumCores(); ++i) {
    uint socket_id = priv::pcm_instance_->getSocketId(i);
    socket_core_count[socket_id]++;
    double ipc = getIPC(priv::core_state_1_[i], priv::core_state_2_[i]);
    this->readings_.overall_usage += ipc;
    this->readings_.socket_usage[socket_id] += ipc;
    this->readings_.core_usage.push_back(ipc);
  }

  this->readings_.overall_usage /= priv::pcm_instance_->getNumCores();

  /* Add energy overall all sockets */
  for (uint i = 0; i < num_sockets; ++i) {
    float energy = pcm::getConsumedJoules(priv::socket_state_1_[i],
                                          priv::socket_state_2_[i]);
    float pwr = energy * 1000 / this->readings_.difference;
    this->readings_.overall_power += pwr;
    this->readings_.overall_energy += energy;
    this->readings_.socket_power.push_back(pwr);
    this->readings_.socket_energy.at(i) += energy;
    this->readings_.socket_usage[i] /= socket_core_count[i];
  }
}

std::vector<Readings*> IntelMeterObserver::GetReadings() {
  return std::vector<Readings*>{static_cast<Readings*>(&(this->readings_))};
}

Status IntelMeterObserver::SelectDevice(const uint /* device */) {
  return Status{Status::NOT_IMPLEMENTED, "Cannot select a device"};
}

Status IntelMeterObserver::SetScope(const ObserverScope scope) {
  if (ObserverScope::SYSTEM == scope) return Status{};
  return Status{Status::NOT_IMPLEMENTED, "The scope is only set to SYSTEM"};
}

Status IntelMeterObserver::SetPID(const uint /* pid */) {
  return Status{Status::NOT_IMPLEMENTED,
                "It is not possible to set a PID in a SYSTEM wide Observer"};
}

ObserverScope IntelMeterObserver::GetScope() const noexcept {
  return ObserverScope::SYSTEM;
}

uint IntelMeterObserver::GetPID() const noexcept { return 0; }

const std::vector<ObserverCapabilities>& IntelMeterObserver::GetCapabilities()
    const noexcept {
  return this->caps_;
}

Status IntelMeterObserver::GetStatus() {
  std::scoped_lock<std::mutex> lock(priv::pcm_mutex_);
  if (priv::pcm_valid_ < 2) {
    return Status{Status::NOT_READY,
                  "The Intel PCM does not have taken enough samples"};
  } else if (!this->valid_) {
    return Status{Status::NOT_READY,
                  "The internal trigger() has not been launched yet"};
  }
  return Status{};
}

Status IntelMeterObserver::SetInterval(const uint64_t interval) {
  this->interval_ = interval;
  return Status{};
}

Status IntelMeterObserver::ClearInterval() {
  return Status{Status::NOT_IMPLEMENTED,
                "The clear interval is not implemented yet"};
}

Status IntelMeterObserver::Reset() {
  std::scoped_lock<std::mutex> lock(priv::pcm_mutex_);
  uint num_sockets = priv::pcm_instance_->getNumSockets();
  this->readings_.type = static_cast<uint>(ObserverType::NONE);
  this->readings_.timestamp = 0;
  this->readings_.difference = 0;
  this->readings_.overall_usage = -1;
  this->readings_.overall_power = -1;
  this->readings_.overall_energy = 0;
  this->readings_.socket_power.clear();
  this->readings_.core_energy.clear();
  this->readings_.core_usage.clear();
  this->readings_.socket_usage.clear();
  this->readings_.socket_energy.resize(num_sockets, 0.f);
  return Status{};
}

IntelMeterObserver::~IntelMeterObserver() {}

} /* namespace efimon */
