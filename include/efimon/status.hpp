/**
 * @file status.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Encapsulates the message
 *
 * @copyright Copyright (c) 2023. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_STATUS_HPP_
#define INCLUDE_EFIMON_STATUS_HPP_

#include <exception>
#include <string>

namespace efimon {

/**
 * @brief Status structure
 *
 * Class that encapsulates the error codes and works for exceptions
 */
class Status : public std::exception {
 public:
  /** Error code for C compatibility */
  int code;
  /** Error message */
  std::string msg;

  /** Error codes */
  enum {
    OK = 0,            /** OK Status */
    FILE_ERROR,        /** File error that can be read or write */
    INVALID_PARAMETER, /** Invalid argument or parameter. i.e. nullptr */
    /** Incompatible parameter that it is not supported
                              by a function */
    INCOMPATIBLE_PARAMETER,
    CONFIGURATION_ERROR,  /** Configuration error*/
    REGISTER_IO_ERROR,    /** Register MMIO error */
    NOT_IMPLEMENTED,      /** Not implemented error */
    MEMBER_ABSENT,        /** Missing member */
    RESOURCE_BUSY,        /** Busy */
    NOT_FOUND,            /** Reosurce not found */
    LOGGER_CANNOT_OPEN,   /** Logger cannot be open */
    LOGGER_CANNOT_INSERT, /** Logger incapable to insert new row */
    NOT_READY,            /** Not ready */
    /** The resource cannot be accessed with current user privileges */
    ACCESS_DENIED,
  };

  /**
   * @brief Construct a new Status object
   *
   * It default the error to be 0 or OK
   */
  Status() noexcept : std::exception{}, code{0} {}

  /**
   * @brief Construct a new Status object
   *
   * It defines the constructor to define a custom code and description
   *
   * @param code code of the error
   * @param msg description
   */
  Status(const int code, const std::string& msg) noexcept
      : std::exception{}, code{code}, msg{msg} {}

  /**
   * @brief Returns the error message from the exception
   *
   * @return const char*
   */
  virtual const char* what() const noexcept { return msg.c_str(); }
};
} /* namespace efimon */
#endif /* INCLUDE_EFIMON_STATUS_HPP_ */
