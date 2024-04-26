/**
 * @file net-readings.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Container interface to hold the metering readings (net)
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_READINGS_NET_READINGS_HPP_
#define INCLUDE_EFIMON_READINGS_NET_READINGS_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include <efimon/readings.hpp>

namespace efimon {

/**
 * @brief Readings specific to network measurements
 */
struct NetReadings : public Readings {
  /** Overall TX volume in KiB */
  float overall_tx_volume;
  /** Overall RX volume in KiB */
  float overall_rx_volume;
  /** Overall TX volume in Packets */
  uint64_t overall_tx_packets;
  /** Overall RX volume in Packets */
  uint64_t overall_rx_packets;
  /** Overall TX band width in KiB */
  float overall_tx_bw;
  /** Overall RX band width in KiB */
  float overall_rx_bw;
  /** Overall TX power in Watts */
  float overall_tx_power;
  /** Overall RX power in Watts */
  float overall_rx_power;
  /** Device names */
  std::string dev_name;
  /** Destructor to enable the inheritance */
  virtual ~NetReadings() = default;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_READINGS_NET_READINGS_HPP_ */
