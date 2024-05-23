/**
 * @file nethogs-test.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of Nethogs library usage
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <mutex>   // NOLINT
#include <thread>  // NOLINT

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <libnethogs.h>
#pragma GCC diagnostic pop

class PendingUpdates final {
  PendingUpdates() = delete;

 public:
  struct Update {
    int action;
    /** Contains all the information about the PID, devname and send/recv kbs */
    NethogsMonitorRecord record;
  };

  /**
   * @brief Updates the Row
   *
   * This updates the map with a new record that will be later use for
   * displays or metrics.
   *
   * @param action
   * @param record
   */
  static void SetRowUpdate(int action, NethogsMonitorRecord const &record) {
    if (action == NETHOGS_APP_ACTION_REMOVE || record.sent_bytes ||
        record.recv_bytes) {
      // save the update for GUI use
      std::lock_guard<std::mutex> lock(m_mutex);
      Update update;
      memset(&update, 0, sizeof(update));
      update.action = action;
      update.record = record;
      m_row_updates_map[record.record_id] = update;
    }
  }

  /**
   * @brief Set the Net Hogs Monitor Status object.
   *
   * It is used by the nethogs thread
   *
   * @param status
   */
  static void SetNetHogsMonitorStatus(int status) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nethogs_monitor_status = status;
  }

  /**
   * @brief Gets the current status of the monitor.
   */
  static int GetNetHogsMonitorStatus() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nethogs_monitor_status;
  }

  /**
   * @brief Get the Row Update object
   *
   * Writes the first object of the map (FIFO) and deletes it from the map
   *
   * @param update [output]
   * @return true if valid
   */
  static bool GetRowUpdate(Update &update) {  // NOLINT
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_row_updates_map.empty()) return false;
    update = m_row_updates_map.begin()->second;
    m_row_updates_map.erase(m_row_updates_map.begin());
    return true;
  }

 private:
  typedef std::map<int, Update> RowUpdatesMap;
  inline static std::mutex m_mutex;
  inline static RowUpdatesMap m_row_updates_map;
  inline static int m_nethogs_monitor_status;
};

static void OnNethogsUpdate(int action, NethogsMonitorRecord const *update) {
  PendingUpdates::SetRowUpdate(action, *update);
}

static void NethogsThread() {
  // Nethogs monitor start: 1000 is the loop time, and nullptr is the filter
  // Which resulted to be experimental. It seems that there is no filter capable
  // to filter by PID.
  const int status = nethogsmonitor_loop(&OnNethogsUpdate, nullptr, 1000);
  PendingUpdates::SetNetHogsMonitorStatus(status);
}

int main() {
  // Check root
  if (0 != geteuid()) {
    std::cerr << "ERROR: This application must be called as root" << std::endl;
    return -1;
  }

  // Launch thread
  std::thread nethogs_monitor_thread(&NethogsThread);

  // Loop around
  while (true) {
    typename PendingUpdates::Update update;
    while (PendingUpdates::GetRowUpdate(update)) {
      if (update.action != NETHOGS_APP_ACTION_REMOVE) {
        std::cout << "PID: " << update.record.pid << " [" << update.record.name
                  << " ]"
                  << " IFace: " << update.record.device_name
                  << " Recv: " << update.record.recv_kbs << " kB/s"
                  << " Sent: " << update.record.sent_kbs << " kB/s"
                  << std::endl;
      }
    }
    sleep(1);
    std::cout << "\033[H\033[2J\033[3J";
  }

  // Break the monitor and close
  nethogsmonitor_breakloop();
  nethogs_monitor_thread.join();

  return 0;
}
