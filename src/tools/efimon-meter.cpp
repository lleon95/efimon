/**
 * @file efimon-meter.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Tool for metering the consumption of existing processes
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <iostream>

#include <efimon/arg-parser.hpp>

int main(int argc, char **argv) {
  auto cli = efimon::ArgParser{argc, argv};

  std::cout << "Program name: " << cli.GetOption("--program-name") << std::endl;

  return 0;
}
