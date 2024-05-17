/**
 * @file csv.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Logger implementation in CSV
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/logger/csv.hpp>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>

#include <efimon/status.hpp>

namespace efimon {

std::string CSVLogger::Stringify(const std::shared_ptr<Logger::IValue> val) {
  switch (val->type) {
    case Logger::FieldType::INTEGER64: {
      auto valc = std::dynamic_pointer_cast<const Logger::Value<int64_t>>(val);
      if (!valc) return std::string{};
      return std::to_string(valc->val);
    }
    case Logger::FieldType::FLOAT: {
      auto valc = std::dynamic_pointer_cast<const Logger::Value<float>>(val);
      if (!valc) return std::string{};
      return std::to_string(valc->val);
    }
    case Logger::FieldType::STRING: {
      auto valc =
          std::dynamic_pointer_cast<const Logger::Value<std::string>>(val);
      if (!valc) return std::string{};
      return std::string("'") + valc->val + std::string("'");
    }
    default:
      break;
  }
  return std::string{};
}

Status CSVLogger::InsertRow(
    const std::unordered_map<std::string, std::shared_ptr<IValue>> & /*vals*/) {
  return Status{};
}

CSVLogger::CSVLogger(const std::string &filename,
                     const std::vector<MapTuple> & /*fields*/)
    : filename_{filename}, table_map_{}, csv_file_{} {}

CSVLogger::~CSVLogger() {}

} /* namespace efimon */
