/**
 * @file sqlite.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Logger implementation in sqlite3
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/logger/sqlite.hpp>

#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>

#include <efimon/status.hpp>

namespace efimon {
static std::unordered_map<Logger::FieldType, std::string> kSqlMapping = {
    {Logger::FieldType::INTEGER, "INT"},
    {Logger::FieldType::FLOAT, "REAL"},
    {Logger::FieldType::STRING, "TEXT"},
};

Status SQLiteLogger::InsertColumn(
    const std::unordered_map<std::string, Logger::IValue> &vals) {
  for (const auto &val : vals) {
    std::cout << "Value type: " << std::get<0>(val) << std::endl;
  }
  return Status{};
}

SQLiteLogger::SQLiteLogger(const std::string &filename,
                           const std::string &session,
                           const std::vector<MapTuple> &fields)
    : filename_{filename}, tablename_{session}, table_map_{} {
  /* Open the database */
  int rc = 0;
  char *zErrMsg = 0;
  this->database_ = nullptr;

  rc = sqlite3_open_v2(filename.c_str(), &this->database_,
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

  if (rc) {
    throw Status{Status::LOGGER_CANNOT_OPEN,
                 std::string(sqlite3_errmsg(this->database_))};
  }

  /* Create the table */
  std::string sql =
      std::string("CREATE TABLE ") + this->tablename_ + std::string("(");
  sql += "ID INT PRIMARY KEY NOT NULL";

  for (auto &field : fields) {
    table_map_[std::get<0>(field)] = std::get<1>(field);
    sql += std::string(", ") + std::string(std::get<0>(field)) +
           std::string(" ") + kSqlMapping[std::get<1>(field)];
  }
  sql += ", Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP";

  sql += ");";
  std::cout << "Table:\n" << sql << std::endl;

  /* Execute SQL statement */
  rc = sqlite3_exec(this->database_, sql.c_str(), nullptr, 0, &zErrMsg);

  if (rc) {
    std::string msg = std::string(zErrMsg);
    sqlite3_free(zErrMsg);
    sqlite3_close(this->database_);
    this->database_ = nullptr;
    throw Status{Status::LOGGER_CANNOT_OPEN, msg};
  }
}

SQLiteLogge::~SQLiteLogger() {
  if (this->database_) {
    sqlite3_close(this->database_);
    this->database_ = nullptr;
  }
}

} /* namespace efimon */
