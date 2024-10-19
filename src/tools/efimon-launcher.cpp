/**
 * @file efimon-launcher.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Wrapper tool for metering the consumption of existing processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <json/json.h>

#include <atomic>
#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <csignal>             // NOLINT
#include <efimon/arg-parser.hpp>
#include <efimon/logger/macros.hpp>
#include <efimon/proc/cpuinfo.hpp>
#include <efimon/process-manager.hpp>
#include <efimon/status.hpp>
#include <memory>
#include <mutex>  // NOLINT
#include <sstream>
#include <string>
#include <thread>  // NOLINT
#include <zmq.hpp>

#include "macro-handling.hpp"  // NOLINT

static constexpr int kThreadCheckTime = 10;   // 10 millis
static constexpr int kThreadStartUpTime = 3;  // 3 seconds

using namespace efimon;  // NOLINT

struct AppData {
  // Manages the app arguments and data
  std::vector<std::string> command;
  uint port = 5550;
  uint pid;
  uint frequency = kDefFrequency;
  uint samples = -1;
  uint delay = kDelay;
  bool enable_perf = false;

  // Manages the process manager
  ProcessManager manager;
  std::mutex manager_mtx;
  std::condition_variable manager_cv;
  std::thread manager_th;
  std::atomic<bool> close;
  std::atomic<bool> terminated;

  // Manages the socket
  std::shared_ptr<zmq::socket_t> socket;
};

// ------------ Application data -------------------
static std::shared_ptr<AppData> appdata_global;

void print_welcome() {
  std::cout << "-----------------------------------------------------------\n";
  std::cout << "               EfiMon Launcher Application                 \n";
  std::cout << "-----------------------------------------------------------\n";
}

std::string get_help(char **argv) {
  std::string msg =
      "This application launches a daemon wrapper for measuring external "
      "applications: EfiMon Launcher\n\tUsage: "
      "\n\t";
  msg += std::string(argv[0]);
  msg +=
      " -s,--samples SAMPLES (default: -1). Number of samples to "
      "collect. -1 means until the process finishes\n\t\t";
  msg +=
      " -f,--frequency FREQUENCY_HZ (default: 100 Hz). Sampling "
      "frequency\n\t\t";
  msg += " -d,--delay DELAY_SECS (default: 3 Secs). Sampling time window\n\t\t";
  msg +=
      " -c,--command COMMAND. Command to execute. This option must be at "
      "the end of the launcher command\n\t\t";
  msg +=
      " -pid,--pid PID. PID to attach to. This option must be at "
      "the end of the launcher command\n\t\t";
  msg +=
      " -p,--port PORT (default: 5550 Secs). EfiMon Socket Port for "
      "IPC\n\t\t";
  msg += " -h,--help: prints this message\n\n";
  msg +=
      " \tBy default, the outputs will be saved into the folder with the "
      "pattern output-pid.csv\n";
  return msg;
}

void launch_command(AppData &data) {  // NOLINT
  // Create the process and launch it
  Status st;
  uint count = data.command.size();
  data.manager_mtx.lock();
  if (count == 1) {
    st = data.manager.Open(data.command[0], ProcessManager::Mode::SILENT);
  } else {
    st = data.manager.Open(data.command[0], data.command,
                           ProcessManager::Mode::SILENT);
  }
  data.manager_mtx.unlock();

  if (Status::OK != st.code) {
    data.manager_mtx.lock();
    data.terminated.store(true);
    data.manager_mtx.unlock();
    data.manager_cv.notify_one();
    return;
  }
  data.manager_cv.notify_one();

  bool local_close = false;
  while (!local_close) {
    if (!data.manager.IsRunning()) {
      data.manager_mtx.lock();
      data.terminated.store(true);
      data.manager_mtx.unlock();
      break;
    }

    // Sleep while running
    std::this_thread::sleep_for(std::chrono::milliseconds(kThreadCheckTime));

    // Check close status
    data.manager_mtx.lock();
    local_close = data.close.load();
    data.manager_mtx.unlock();
  }
  data.manager_mtx.lock();
  data.manager.Close();
  data.manager_mtx.unlock();
}

Json::Value create_template(const AppData &data) {
  Json::Value root;

  root["transaction"] = "process";
  root["state"] = true;
  root["pid"] = 0;

  root["perf"] = data.enable_perf;
  root["frequency"] = data.frequency;
  root["samples"] = data.samples;
  root["delay"] = data.delay;

  return root;
}

void start_monitor(const AppData &data) {  // NOLINT
  Json::StreamWriterBuilder wbuilder;
  std::string str_message, str_err;
  Json::CharReaderBuilder rbuilder;
  Json::Value payload, res_json;
  std::stringstream res_str;
  bool res_ok = false;

  [[maybe_unused]] zmq::send_result_t send_res;
  [[maybe_unused]] zmq::recv_result_t recv_res;

  if (!data.socket->connected()) {
    // Error already reported
    return;
  }

  payload = create_template(data);
  rbuilder["collectComments"] = false;

  /* Initialise the system monitor */
  payload["transaction"] = "system";
  payload["state"] = true;

  /* Send the message */
  str_message = Json::writeString(wbuilder, payload);
  zmq::message_t system_msg(str_message);
  send_res = data.socket->send(system_msg, zmq::send_flags::none);
  recv_res = data.socket->recv(system_msg, zmq::recv_flags::none);
  res_str << system_msg.to_string();
  res_ok = Json::parseFromStream(rbuilder, res_str, &res_json, &str_err);
  if (res_ok) {
    res_ok = res_json.isMember("result") ? res_json["result"] == "" : false;
    if (res_ok) {
      EFM_INFO("System Monitor started");
    } else {
      EFM_INFO(
          "System Monitor could not be started. Probably, it's been started "
          "before");
    }
  } else {
    EFM_WARN("Cannot parse the response: " + str_err);
  }

  /* Initialise the process monitor */
  payload["transaction"] = "process";
  payload["pid"] = data.pid;

  /* Send the message */
  str_message = Json::writeString(wbuilder, payload);
  zmq::message_t process_msg(str_message);
  send_res = data.socket->send(process_msg, zmq::send_flags::none);
  recv_res = data.socket->recv(process_msg, zmq::recv_flags::none);
  res_str << process_msg.to_string();
  res_ok = Json::parseFromStream(rbuilder, res_str, &res_json, &str_err);
  if (res_ok) {
    res_ok = res_json.isMember("result") ? res_json["result"] == "" : false;
    if (res_ok) {
      EFM_INFO("Process Monitor started");
    } else {
      EFM_INFO("Process Monitor could not be started");
    }
  } else {
    EFM_WARN("Cannot parse the response" + str_err);
  }
}

Status check_monitor(const AppData &data) {
  Json::StreamWriterBuilder wbuilder;
  std::string str_message, str_err;
  Json::CharReaderBuilder rbuilder;
  Json::Value payload, res_json;
  std::stringstream res_str;
  bool res_ok = false;

  [[maybe_unused]] zmq::send_result_t send_res;
  [[maybe_unused]] zmq::recv_result_t recv_res;

  /* Make the payload */
  payload["transaction"] = "poll";
  payload["pid"] = data.pid;

  /* Send the message */
  str_message = Json::writeString(wbuilder, payload);
  zmq::message_t process_msg(str_message);
  send_res = data.socket->send(process_msg, zmq::send_flags::none);
  recv_res = data.socket->recv(process_msg, zmq::recv_flags::none);
  res_str << process_msg.to_string();
  res_ok = Json::parseFromStream(rbuilder, res_str, &res_json, &str_err);

  /* Check response */
  if (!res_ok || !res_json.isMember("result")) {
    return Status{Status::INVALID_PARAMETER, "The response is invalid"};
  }

  /* Check response: we expect a numerical response */
  std::string value = res_json["result"].asString();
  bool numeric = std::all_of(value.begin(), value.end(), ::isdigit);
  if (!numeric) {
    EFM_WARN("The response when polling is invalid. Value is: " + value);
    return Status{Status::INVALID_PARAMETER, "The response is invalid"};
  }

  int val = std::stoi(value);
  return Status{val, ""};
}

void stop_monitor(const AppData &data) {  // NOLINT
  Json::StreamWriterBuilder wbuilder;
  std::string str_message, str_err;
  Json::CharReaderBuilder rbuilder;
  Json::Value payload, res_json;
  std::stringstream res_str;
  bool res_ok = false;

  [[maybe_unused]] zmq::send_result_t send_res;
  [[maybe_unused]] zmq::recv_result_t recv_res;

  if (!data.socket->connected()) {
    // Error already reported
    return;
  }

  payload = create_template(data);
  rbuilder["collectComments"] = false;

  /* Stop the process monitor */
  payload["transaction"] = "process";
  payload["pid"] = data.pid;
  payload["state"] = false;

  /* Send the message */
  str_message = Json::writeString(wbuilder, payload);
  zmq::message_t process_msg(str_message);
  send_res = data.socket->send(process_msg, zmq::send_flags::none);
  recv_res = data.socket->recv(process_msg, zmq::recv_flags::none);
  res_str << process_msg.to_string();
  res_ok = Json::parseFromStream(rbuilder, res_str, &res_json, &str_err);
  if (res_ok) {
    res_ok = res_json.isMember("result") ? res_json["result"] == "" : false;
    if (res_ok) {
      EFM_INFO("Process Monitor stopped");
    } else {
      EFM_INFO("Process Monitor could not be stopped. Payload: " +
               process_msg.to_string());
    }
  } else {
    EFM_WARN("Cannot parse the response" + str_err);
  }
}

void signal_handler(int /*signal*/) {
  EFM_WARN("Termination signal received");
  appdata_global->close.store(true);
}

int main(int argc, char **argv) {
  print_welcome();
  AppData appdata{};
  appdata_global = decltype(appdata_global){&appdata, [](void *) {}};

  // ------------ Comm data -------------------
  zmq::context_t context;
  auto type = zmq::socket_type::req;
  appdata.socket = std::make_shared<zmq::socket_t>(context, type);

  // ------------ Arguments ------------
  ArgParser argparser(argc, argv);

  bool check_frequency =
      argparser.Exists("-f") || argparser.Exists("--frequency");
  bool check_samples = argparser.Exists("-s") || argparser.Exists("--samples");
  bool check_delay = argparser.Exists("-d") || argparser.Exists("--delay");
  bool check_help = argparser.Exists("-h") || argparser.Exists("--help");
  bool check_port = argparser.Exists("-p") || argparser.Exists("--port");
  bool check_command = argparser.Exists("-c") || argparser.Exists("--command");
  bool check_pid = argparser.Exists("-pid") || argparser.Exists("--pid");
  // TODO(lleon): add enable perf option

  if (check_help) {
    std::string msg = get_help(argv);
    EFM_ERROR(msg);
  }

  if (!check_command && !check_pid) {
    EFM_ERROR("Cannot execute without a command or a PID");
    std::string msg = get_help(argv);
    EFM_ERROR(msg);
  }

  // ------------ Modify the configurations -----------
  if (check_command) {
    // Get the arguments from the command
    auto bit = argparser.GetBegin("-c");
    auto eit = argparser.GetEnd();
    const int count = eit - bit;
    appdata.command.resize(count);
    std::copy(bit, eit, appdata.command.begin());
  } else {
    appdata.command.clear();
    appdata.pid =
        std::stoi(argparser.Exists("-pid") ? argparser.GetOption("-pid")
                                           : argparser.GetOption("--pid"));
  }

  if (check_samples) {
    appdata.samples =
        std::stoi(argparser.Exists("-s") ? argparser.GetOption("-s")
                                         : argparser.GetOption("--samples"));
  }

  if (check_frequency) {
    appdata.frequency =
        std::stoi(argparser.Exists("-f") ? argparser.GetOption("-f")
                                         : argparser.GetOption("--frequency"));
  }

  if (check_delay) {
    appdata.delay =
        std::stoi(argparser.Exists("-d") ? argparser.GetOption("-d")
                                         : argparser.GetOption("--delay"));
  }

  if (check_port) {
    appdata.port =
        std::stoi(argparser.Exists("-p") ? argparser.GetOption("-p")
                                         : argparser.GetOption("--port"));
  }

  EFM_INFO(std::string("Frequency [Hz]: ") + std::to_string(appdata.frequency));
  EFM_INFO(std::string("Samples: ") + std::to_string(appdata.samples));
  EFM_INFO(std::string("Delay time [secs]: ") + std::to_string(appdata.delay));
  EFM_INFO(std::string("IPC TCP Port: ") + std::to_string(appdata.port));

  // Launch the Process
  appdata.terminated.store(false);
  appdata.close.store(false);
  if (check_command) {
    EFM_INFO("Launching the process with command: " + appdata.command[0]);
    appdata.manager_th = std::thread(launch_command, std::ref(appdata));
    {
      std::unique_lock lk(appdata.manager_mtx);
      appdata.manager_cv.wait_for(lk, std::chrono::seconds(kThreadStartUpTime));
      EFM_INFO("Launched command: " + appdata.command[0]);
    }
  } else {
    EFM_INFO("Launching the listener with PID: " + std::to_string(appdata.pid));
  }

  // Attach signal handling for early termination
  std::signal(SIGINT, signal_handler);

  if (appdata.terminated.load()) {
    // The process terminated early or abnormally
    EFM_ERROR(
        "The process cannot be monitored. The termination activated early");
    return -1;
  }

  // Connect to the socket
  std::string endpoint = "tcp://localhost:" + std::to_string(appdata.port);
  EFM_INFO("Connecting to daemon over " + endpoint);
  appdata.socket->connect(endpoint);
  if (!appdata.socket->connected()) {
    EFM_WARN("Cannot connect to the monitoring daemon");
  } else {
    EFM_INFO("Connected to the monitoring daemon");
  }

  // Start the monitor
  appdata.pid = appdata.manager.GetPID();
  start_monitor(appdata);
  // TODO(lleon): get name
  // TODO(lleon): add timestamp to logs

  while (!appdata.terminated.load()) {
    // Wait according to the delay, which is often smaller than the one
    // from the daemon
    std::this_thread::sleep_for(std::chrono::seconds(appdata.delay));

    Status res = check_monitor(appdata);

    if (Status::STOPPED == res.code) {
      EFM_INFO("The monitor has completed the number of samples");
      break;
    }
  }

  if (appdata.terminated.load()) {
    EFM_INFO("Process stopped normally. Stopping monitor");
  } else {
    EFM_INFO("Sending termination signal. Stopping monitor");
    appdata.close.store(true);
  }

  // Stop the monitor
  stop_monitor(appdata);

  // Close the socket
  appdata.socket.reset();

  // Join the Process
  appdata.manager_th.join();
  EFM_INFO("Finished. Closing everything...");
  return 0;
}
