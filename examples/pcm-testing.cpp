/**
 * @file pcm-testing
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Proof of Concept Snippet for Intel PCM
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <unistd.h>

#include <iostream>
#include <third-party/pcm.hpp>

int main() {
  std::cout << "Hello from PCM" << std::endl;

  std::cout << "INFO: Getting Instance" << std::endl;
  pcm::PCM* m = pcm::PCM::getInstance();

  std::cout << "INFO: Mutex" << std::endl;
  const pcm::PCM::ErrorCode status =
      m->program(pcm::PCM::DEFAULT_EVENTS, nullptr, false, -1);
  if (pcm::PCM::Success != status) {
    std::cerr << "ERROR: Cannot launch the program. Is it root?" << std::endl;
  }

  std::vector<pcm::CoreCounterState> cstates1, cstates2;
  std::vector<pcm::SocketCounterState> sktstate1, sktstate2;
  pcm::SystemCounterState sstate1, sstate2;

  std::cout << "INFO: Getting 1 sec of data" << std::endl;
  m->getAllCounterStates(sstate1, sktstate1, cstates1);
  sleep(1);
  m->getAllCounterStates(sstate2, sktstate2, cstates2);
  sleep(1);

  std::cout << "INFO: Printing data" << std::endl;
  for (uint i = 0; i < m->getNumSockets(); ++i) {
    if (m->packageEnergyMetricsAvailable()) {
      std::cout << "Socket " << i << " Energy (Joules): "
                << pcm::getConsumedJoules(sktstate1[0], sktstate2[0])
                << std::endl;
    }
  }
  return 0;
}
