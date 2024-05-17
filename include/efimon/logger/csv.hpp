/**
 * @copyright Copyright (c) 2024. See License for Licensing
 *
 * @file csv.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Logger implementation in filesystem using CSV
 */

#ifndef INCLUDE_EFIMON_LOGGER_CSV_HPP_
#define INCLUDE_EFIMON_LOGGER_CSV_HPP_

#include <fstream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <efimon/logger.hpp>
#include <efimon/status.hpp>

namespace efimon {
class CSVLogger : public Logger {
 public:
  CSVLogger(const std::string &filename, const std::vector<MapTuple> &fields);

  Status InsertRow(
      const std::unordered_map<std::string, std::shared_ptr<Logger::IValue>>
          &vals) override;

  virtual ~CSVLogger();

 private:
  std::string filename_;
  std::unordered_map<std::string, FieldType> table_map_;
  std::ofstream csv_file_;

  std::string Stringify(const std::shared_ptr<Logger::IValue> val);
};
} /* namespace efimon */

#endif /* INCLUDE_EFIMON_LOGGER_CSV_HPP_ */
