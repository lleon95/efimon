/**
 * @file logger.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Logger interface class
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_LOGGER_HPP_
#define INCLUDE_EFIMON_LOGGER_HPP_

#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include <efimon/status.hpp>

namespace efimon {
class Logger {
 public:
  /**
   * @brief Field Type of the table
   *
   * Determines the type of the field to add. This is required for typing the
   * table.
   */
  enum class FieldType {
    /** The field does not have a type and it is treated as binary */
    NONE = 0,
    /** The field is an integer */
    INTEGER64,
    /** The field is a floating-point / real number */
    FLOAT,
    /** The field is a string */
    STRING,
  };

  struct IValue {
    FieldType type;
    virtual void Placeholder() = 0;
    virtual ~IValue() = default;
  };

  template <class T>
  struct Value : public IValue {
    T val;

    explicit Value(const T val_a) {
      this->val = T{};
      if constexpr (std::is_integral<T>::value)
        this->type = FieldType::INTEGER64;
      else if constexpr (std::is_floating_point<T>::value)
        this->type = FieldType::FLOAT;
      else if constexpr (std::is_same<T, std::string>::value)
        this->type = FieldType::STRING;
      else
        this->type = FieldType::NONE;
      val = val_a;
    }

    void operator()(const T value) noexcept { val = value; }

    void Placeholder() override {}

    virtual ~Value() = default;
  };

  /**
   * @brief Type for the mapper
   *
   * It defines a tuple that will help to map each field added during
   * the logging
   */
  typedef std::tuple<std::string, FieldType> MapTuple;

  /*
    Logger(const std::string & filename,
           const std::string & session,
           const std::vector<MapTuple> &fields);
  */

  virtual Status InsertColumn(
      const std::unordered_map<std::string, std::shared_ptr<IValue>> &vals) = 0;

  virtual ~Logger() = default;
};
} /* namespace efimon */

#endif /* INCLUDE_EFIMON_LOGGER_HPP_ */
