/**
 * @file arg-parser.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief A really basic CLI Argument Parser
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_ARG_PARSER_HPP_
#define INCLUDE_EFIMON_ARG_PARSER_HPP_

#include <string>
#include <vector>

namespace efimon {

/**
 * @brief Argument Parser Class
 *
 * This is an oversimplied argument parser tool to parse the options coming
 * from the CLI. The usage is almost trivial. Construct the object by using
 * the constructor and passing the arguments, check if the option exists and
 * get the option value.
 *
 * Relevant: this argument parser assumes the syntax "--option value". As such,
 * this does not support switch-only arguments.
 */
class ArgParser {
 public:
  /**
   * @brief Default constructor: deleted
   *
   * We want to generate the ArgParser from the CLI args directly
   */
  ArgParser() = delete;

  /**
   * @brief Construct a new Argument Parser object
   *
   * This class provides an easy interface for parsing the CLI arguments for
   * internal tooling.
   *
   * @param argc number of arguments coming from the main entrypoint
   * @param argv array of char string with the arguments coming from the main
   * entrypoint
   */
  ArgParser(int argc, char **argv) noexcept;

  /**
   * @brief Checks whether an argument/option is present or not
   *
   * The usage of this method has two main goals:
   *
   * 1. Check if a switch is enabled (a switch is an option without value)
   *
   * 2. Check if an option effectively exists.
   *
   * @param option option to check
   * @return true if exists.
   */
  bool Exists(const std::string &option) const noexcept;

  /**
   * @brief Gets a string with the option
   *
   * This method must be used only for options with value. Using this method
   * with switches may lead to incorrect results.
   *
   * @param option option to retrieve
   * @return const std::string&
   * @throw std::runtime_exception if the option does not exist
   */
  const std::string &GetOption(const std::string &option) const;

  /**
   * @brief Destroy the Arg Parser object
   */
  virtual ~ArgParser() = default;

 private:
  /** Vector with the parsed arguments */
  std::vector<std::string> arguments_;
};

} /* namespace efimon */

#endif /* INCLUDE_EFIMON_ARG_PARSER_HPP_ */
