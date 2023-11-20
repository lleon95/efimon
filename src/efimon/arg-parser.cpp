/**
 * @file arg-parser.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief A really basic CLI Argument Parser (implementation)
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#include <efimon/arg-parser.hpp>

#include <algorithm>
#include <stdexcept>

namespace efimon {
ArgParser::ArgParser(int argc, char **argv) noexcept {
  /* Initialise the vector with the "program-name".
     Thus, it makes the first argument an option with value */
  arguments_.push_back("--program-name");

  /* Add all the arguments as strings to the arguments vector */
  for (int i = 0; i < argc; ++i) {
    arguments_.push_back(std::string{argv[i]});
  }
}

bool ArgParser::Exists(const std::string &option) const noexcept {
  return (std::find(arguments_.begin(), arguments_.end(), option) !=
          arguments_.end());
}

const std::string &ArgParser::GetOption(const std::string &option) const {
  /* Check if it exists */
  if (!this->Exists(option)) {
    throw std::runtime_error("Cannot get non-existing option");
  }

  /* Find the option */
  auto itr = std::find(arguments_.begin(), arguments_.end(), option);

  /* Get the value */
  itr++;
  if (arguments_.end() == itr) {
    throw std::runtime_error("Switched tried to be accessed as option-value");
  }

  return *itr;
}
} /* namespace efimon */
