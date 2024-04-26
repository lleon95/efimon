/**
 * @file efimon-meter.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Tool for metering the consumption of existing processes
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <unistd.h>

#include <iostream>

#include <efimon/arg-parser.hpp>
#include <efimon/observer-enums.hpp>
#include <efimon/proc/io.hpp>
#include <efimon/proc/meminfo.hpp>
#include <efimon/proc/net.hpp>
#include <efimon/proc/stat.hpp>
#include <efimon/proc/thread-tree.hpp>

int main(int argc, char **argv) {
  auto cli = efimon::ArgParser{argc, argv};

  if (!cli.Exists("--pid")) {
    std::cerr << "--pid option not given and it is mandatory" << std::endl;
    return -1;
  }

  int pid = std::stoi(cli.GetOption("--pid"));

  std::cout << "Program name: " << cli.GetOption("--program-name") << std::endl;
  std::cout << "PID: " << pid << std::endl;

  efimon::ThreadTree ptree{pid};

  std::cout << "--- Creating process tree ---" << std::endl;
  for (const auto &elem : ptree.GetTree()) {
    std::cout << "\t" << elem << std::endl;
  }

  std::cout << "--- Reading metrics every second for 15 seconds ---"
            << std::endl;
  efimon::ProcStatObserver pprocstat{(uint)pid, efimon::ObserverScope::PROCESS,
                                     1};
  efimon::ProcStatObserver psysstat{0, efimon::ObserverScope::SYSTEM, 1};
  efimon::ProcMemInfoObserver pmeminfo{0, efimon::ObserverScope::SYSTEM, 1};
  efimon::ProcIOObserver pio{(uint)pid, efimon::ObserverScope::PROCESS, 1};
  efimon::ProcNetObserver pnet{0, efimon::ObserverScope::SYSTEM, 1};

  std::cout << "\nReadings: " << std::endl;
  for (int i = 0; i <= 15; ++i) {
    psysstat.Trigger();
    pprocstat.Trigger();
    pio.Trigger();
    pmeminfo.Trigger();
    pnet.Trigger();
    auto netreadings = pnet.GetReadings();
    efimon::CPUReadings *readingcpu =
        dynamic_cast<efimon::CPUReadings *>(pprocstat.GetReadings()[0]);
    efimon::CPUReadings *readingsyscpu =
        dynamic_cast<efimon::CPUReadings *>(psysstat.GetReadings()[0]);
    efimon::RAMReadings *readingsysram =
        dynamic_cast<efimon::RAMReadings *>(pmeminfo.GetReadings()[0]);
    efimon::RAMReadings *readingram =
        dynamic_cast<efimon::RAMReadings *>(pprocstat.GetReadings()[1]);
    efimon::IOReadings *readingio =
        dynamic_cast<efimon::IOReadings *>(pio.GetReadings()[0]);
    std::cout << "\tTotal CPU: " << readingsyscpu->overall_usage << "%: ";
    for (const auto val : readingsyscpu->core_usage) {
      std::cout << val << "% ";
    }
    std::cout << "\tProcess CPU: " << readingcpu->overall_usage << "%";
    std::cout << "\n\tTotal RAM: usage: " << readingsysram->overall_usage
              << " MiB, Process RAM usage: " << readingram->overall_usage
              << " MiB";
    std::cout << "\n\tI/O Read Vol: " << readingio->read_volume << " KiB, ";
    std::cout << "I/O Write Vol: " << readingio->write_volume << " KiB, ";
    std::cout << "I/O Read BW: " << readingio->read_bw << " KiB/s, ";
    std::cout << "I/O Write BW: " << readingio->write_bw << " KiB/s"
              << std::endl;
    for (auto preading : netreadings) {
      efimon::NetReadings *readingnet =
          dynamic_cast<efimon::NetReadings *>(preading);
      std::cout << "\tNetIface: " << readingnet->dev_name
                << ": TX Vol: " << readingnet->overall_tx_volume
                << " KiB, RX Vol: " << readingnet->overall_rx_volume
                << " KiB, TX BW: " << readingnet->overall_tx_bw
                << " KiB/sec, RX BW: " << readingnet->overall_rx_bw
                << " KiB/sec" << std::endl;
    }
    std::cout << "\n\tDifference: " << readingcpu->difference << " ms, ";
    std::cout << "Timestamp: " << readingcpu->timestamp << " ms" << std::endl
              << std::endl;
    sleep(1);
  }

  return 0;
}
