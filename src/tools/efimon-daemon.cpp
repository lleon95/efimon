/**
 * @file efimon-meter.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Tool for metering the consumption of existing processes
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/arg-parser.hpp>
#include <efimon/logger/macros.hpp>
#include <zmqpp/zmqpp.hpp>

using namespace efimon;  // NOLINT

static constexpr int kDelay = 3;                  // 3 seconds
static constexpr uint kDefFrequency = 100;        // 100 Hz
static constexpr uint kDefaultSampleLimit = 100;  // 100 samples
static constexpr char kDefaultOutputPath[] = "/tmp";
static constexpr uint kPort = 5550;

void print_welcome() {
  std::cout << "-----------------------------------------------------------\n";
  std::cout << "               EfiMon Daemon Application \n";
  std::cout << "-----------------------------------------------------------\n";
}

std::string create_monitoring_file(const std::string &path, const uint pid) {
  return path + "-" + std::to_string(pid) + ".csv";
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
  zmqpp::context context;
  zmqpp::socket_type type = zmqpp::socket_type::reply;
  zmqpp::socket socket{context, type};
  socket.bind(endpoint);

  // ----------- Listen forever -----------
  while (true) {
    zmqpp::message message;
    std::string text;

    // Receive message
    socket.receive(message);
    message >> text;

    // Do some work

    // Reply
    EFM_INFO("Received: " + text);
    socket.send("OK");
  }

  return 0;
}
