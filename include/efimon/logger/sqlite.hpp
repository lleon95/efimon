/**
 * @copyright Copyright (c) 2024. See License for Licensing
 *
 * @file sqlite.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Logger implementation in sqlite3
 */

#ifndef INCLUDE_EFIMON_LOGGER_SQLITE_HPP_
#define INCLUDE_EFIMON_LOGGER_SQLITE_HPP_

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <sqlite3.h>

#include <efimon/logger.hpp>
#include <efimon/status.hpp>

namespace efimon {
class SQLiteLogger : public Logger {
 public:
  SQLiteLogger(const std::string &filename, const std::string &session,
               const std::vector<MapTuple> &fields);

  Status InsertColumn(
      const std::unordered_map<std::string, Logger::IValue> &vals) override;

  virtual ~SQLiteLogger();

 private:
  std::string filename_;
  std::string tablename_;
  std::unordered_map<std::string, FieldType> table_map_;
  sqlite3 *database_;
};
} /* namespace efimon */

#endif /* INCLUDE_EFIMON_LOGGER_SQLITE_HPP_ */
