/**
 * @file csv.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Logger implementation in CSV
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <cstdint>
#include <efimon/logger/csv.hpp>
#include <efimon/status.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>

namespace efimon {

std::string CSVLogger::Stringify(const std::shared_ptr<Logger::IValue> val) {
  switch (val->type) {
    case Logger::FieldType::INTEGER64: {
      auto valc = std::dynamic_pointer_cast<const Logger::Value<int64_t>>(val);
      if (!valc) return std::string{"0"};
      return std::to_string(valc->val);
    }
    case Logger::FieldType::FLOAT: {
      auto valc = std::dynamic_pointer_cast<const Logger::Value<float>>(val);
      if (!valc) return std::string{"0.0"};
      return std::to_string(valc->val);
    }
    case Logger::FieldType::STRING: {
      auto valc =
          std::dynamic_pointer_cast<const Logger::Value<std::string>>(val);
      if (!valc) return std::string{};
      return valc->val;
    }
    default:
      break;
  }
  return std::string{};
}

Status CSVLogger::InsertRow(
    const std::unordered_map<std::string, std::shared_ptr<IValue>> &vals) {
  /* Check the file */
  if (!this->csv_file_.is_open()) {
    return Status{Status::LOGGER_CANNOT_INSERT,
                  "Cannot insert since the file is not opened"};
  }

  /* Add row */
  this->csv_file_ << (this->last_id_++);
  for (const auto &field : table_map_) {
    std::string msg = "";

    if (vals.find(field.first) != vals.end()) {
      msg = Stringify(vals.at(field.first));
    }

    this->csv_file_ << "," << msg;
  }
  this->csv_file_ << std::endl;

  return vals.size() == this->table_map_.size()
             ? Status{}
             : Status{Status::OK, "Not all the fields were present"};
}

CSVLogger::CSVLogger(const std::string &filename,
                     const std::vector<MapTuple> &fields)
    : filename_{filename}, table_map_{}, csv_file_{}, last_id_{0} {
  /* Open the file */
  this->csv_file_.open(filename_);

  if (!this->csv_file_.is_open()) {
    throw Status{Status::LOGGER_CANNOT_OPEN, "The file cannot be opened"};
  }

  /* Create schema */
  for (auto &field : fields) {
    table_map_[std::get<0>(field)] = std::get<1>(field);
  }

  /* Add header */
  this->csv_file_ << "ID";
  for (const auto &field : table_map_) {
    this->csv_file_ << "," << field.first;
  }
  this->csv_file_ << std::endl;
}

CSVLogger::~CSVLogger() { this->csv_file_.close(); }
} /* namespace efimon */
