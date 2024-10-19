/**
 * @file efimon-daemon.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Daemon tool for metering the consumption of existing processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <json/json.h>

#include <efimon/arg-parser.hpp>
#include <efimon/logger/macros.hpp>
#include <efimon/proc/cpuinfo.hpp>
#include <sstream>
#include <zmq.hpp>

#include "efimon-daemon/efimon-analyser.hpp"  // NOLINT
#include "efimon-daemon/efimon-worker.hpp"    // NOLINT
#include "macro-handling.hpp"                 // NOLINT

using namespace efimon;  // NOLINT

void print_welcome() {
  std::cout << "-----------------------------------------------------------\n";
  std::cout << "               EfiMon Daemon Application \n";
  std::cout << "-----------------------------------------------------------\n";
}

std::string create_monitoring_file(const std::string &path, const uint pid) {
  logcounter++;
  return path + "/efimon-" + std::to_string(pid) + "-" +
         std::to_string(logcounter) + ".csv";
}

int main(int argc, char **argv) {
  print_welcome();
  // ------------ Configuration variables ------------
  uint frequency = kDefFrequency;
  uint samples = kDefaultSampleLimit;
  uint delaytime = kDelay;
  std::string outputpath = kDefaultOutputPath;
  uint port = kPort;

  // ------------ Arguments ------------
  ArgParser argparser(argc, argv);

  bool check_frequency =
      argparser.Exists("-f") || argparser.Exists("--frequency");
  bool check_samples = argparser.Exists("-s") || argparser.Exists("--samples");
  bool check_delay = argparser.Exists("-d") || argparser.Exists("--delay");
  bool check_output =
      argparser.Exists("-o") || argparser.Exists("--output-folder");
  bool check_help = argparser.Exists("-h") || argparser.Exists("--help");
  bool check_port = argparser.Exists("-p") || argparser.Exists("--port");

  if (check_help) {
    std::string msg =
        "This application launches a daemon listener for measuring external "
        "applications: EfiMon Daemon\n\tUsage: "
        "\n\t";
    msg += std::string(argv[0]);
    msg +=
        " -s,--samples SAMPLES (default: 100). Number of samples to "
        "collect\n\t\t";
    msg +=
        " -o,--output-folder PATH (default: /tmp). Output folder to save "
        "measurements\n\t\t";
    msg +=
        " -f,--frequency FREQUENCY_HZ (default: 100 Hz). Sampling "
        "frequency\n\t\t";
    msg +=
        " -d,--delay DELAY_SECS (default: 3 Secs). Sampling time window\n\t\t";
    msg +=
        " -p,--port PORT (default: 5550 Secs). EfiMon Socket Port for "
        "IPC\n\t\t";
    msg += " -h,--help: prints this message\n\n";
    msg +=
        " \tBy default, the outputs will be saved into the folder with the "
        "pattern output-pid.csv\n";
    EFM_ERROR(msg);
  }

  // ------------ Modify the configurations -----------
  if (check_samples) {
    samples =
        std::stoi(argparser.Exists("-s") ? argparser.GetOption("-s")
                                         : argparser.GetOption("--samples"));
  }

  if (check_frequency) {
    frequency =
        std::stoi(argparser.Exists("-f") ? argparser.GetOption("-f")
                                         : argparser.GetOption("--frequency"));
  }

  if (check_delay) {
    delaytime =
        std::stoi(argparser.Exists("-d") ? argparser.GetOption("-d")
                                         : argparser.GetOption("--delay"));
  }

  if (check_port) {
    port = std::stoi(argparser.Exists("-p") ? argparser.GetOption("-p")
                                            : argparser.GetOption("--port"));
  }

  if (check_output) {
    outputpath = argparser.Exists("-o")
                     ? argparser.GetOption("-o")
                     : argparser.GetOption("--output-folder");
  }

  EFM_INFO(std::string("Frequency [Hz]: ") + std::to_string(frequency));
  EFM_INFO(std::string("Samples: ") + std::to_string(samples));
  EFM_INFO(std::string("Delay time [secs]: ") + std::to_string(delaytime));
  EFM_INFO(std::string("Output folder: ") + outputpath);
  EFM_INFO(std::string("IPC TCP Port: ") + std::to_string(port));

  // ---------- Initialise ZeroMQ ------------
  std::string endpoint = "tcp://*:" + std::to_string(port);
  zmq::context_t context;
  auto type = zmq::socket_type::rep;
  zmq::socket_t socket{context, type};
  socket.bind(endpoint);

  // ----------- Start the thread -----------
  EfimonAnalyser analyser{};
  analyser.StartSystemThread(delaytime);

  // ----------- Listen forever -----------
  Json::CharReaderBuilder rbuilder;
  Json::StreamWriterBuilder wbuilder;
  Json::Value root;
  rbuilder["collectComments"] = false;

  while (true) {
    Json::Value response;
    zmq::message_t message;
    std::string text, errs, str_message;
    std::stringstream streamtext;

    response["result"] = "";

    // Receive message
    auto res = socket.recv(message, zmq::recv_flags::none);
    res.reset();  // TODO(lleon): improve this handling
    text = message.to_string();
    streamtext << text;

    // Do some work
    bool ok = Json::parseFromStream(rbuilder, streamtext, &root, &errs);
    if (!ok) {
      EFM_WARN("Error while Json: " + errs);
      zmq::message_t reply(std::string("{\"result\": \"Cannot parse\"}"));
      socket.send(reply, zmq::send_flags::none);
    } else {
      Status status{Status::OK, "OK"};

      // Check transaction
      if (!root.isMember("transaction")) {
        EFM_WARN("'transaction' member does not exist");
        zmq::message_t reply(
            std::string("{\"result\": \"Cannot find transaction\"}"));
        socket.send(reply, zmq::send_flags::none);
      }
      std::string transaction = root["transaction"].asString();

      // Complete transaction
      if ("system" == transaction && root.isMember("state")) {
        bool state = root["state"].asBool();
        EFM_INFO("Setting System Monitor to: " + std::to_string(state));
        if (state) {
          status = analyser.StartSystemThread(delaytime);
        } else {
          status = analyser.StopSystemThread();
        }
      } else if ("process" == transaction && root.isMember("state") &&
                 root.isMember("pid")) {
        bool state = root["state"].asBool();
        uint pid = root["pid"].asUInt();
        uint delay =
            root.isMember("delay") ? root["delay"].asUInt() : delaytime;
        bool perf = root.isMember("perf") ? root["perf"].asBool() : false;
        uint freq = root.isMember("frequency") ? root["frequency"].asUInt()
                                               : kDefFrequency;
        uint samples = root.isMember("samples") ? root["samples"].asUInt() : 0;
        std::string name = create_monitoring_file(outputpath, pid);
        name = root.isMember("name") ? root["name"].asString() : name;
        EFM_INFO("Setting Process Monitor to PID " + std::to_string(pid) +
                 " to: " + std::to_string(state) +
                 " with delay: " + std::to_string(delay) + " secs");
        if (state) {
          status =
              analyser.StartWorkerThread(name, pid, delay, samples, perf, freq);
        } else {
          status = analyser.StopWorkerThread(pid);
        }

        response["name"] = name;
      } else if ("poll" == transaction && root.isMember("pid")) {
        uint pid = root["pid"].asUInt();
        status = analyser.CheckWorkerThread(pid);
      } else {
        status = Status{Status::INVALID_PARAMETER, "Invalid set of params"};
      }

      if (Status::OK != status.code) {
        EFM_WARN(status.what());
      }

      response["result"] = status.what();
      response["code"] = status.code;
      str_message = Json::writeString(wbuilder, response);
      zmq::message_t reply(str_message);
      socket.send(reply, zmq::send_flags::none);
    }
  }

  return 0;
}
