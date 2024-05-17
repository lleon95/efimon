/**
 * @file sqlite.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Logger implementation in sqlite3
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/logger/sqlite.hpp>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>

#include <efimon/status.hpp>

namespace efimon {
static std::unordered_map<Logger::FieldType, std::string> kSqlMapping = {
    {Logger::FieldType::INTEGER64, "INT64"},
    {Logger::FieldType::FLOAT, "REAL"},
    {Logger::FieldType::STRING, "TEXT"},
};

std::string SQLiteLogger::Stringify(const std::shared_ptr<Logger::IValue> val) {
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

Status SQLiteLogger::InsertRow(
    const std::unordered_map<std::string, std::shared_ptr<IValue>> &vals) {
  std::string sql = std::string("INSERT INTO ") + this->tablename_;
  std::string fields = "";
  std::string values = "";
  char *zErrMsg = 0;
  int rc = 0;

  for (const auto &val : vals) {
    /* Fill fields */
    if (!fields.empty()) fields += ",";
    fields += std::get<0>(val);

    /* Fill values */
    if (!values.empty()) values += ",";
    values += Stringify(std::get<1>(val));
  }

  /* Complete SQL statement */
  sql += std::string(" (") + fields + std::string(") ");
  sql += std::string("VALUES (") + values + ");";

  /* Execute SQL statement */
  rc = sqlite3_exec(this->database_, sql.c_str(), nullptr, 0, &zErrMsg);
  if (rc) {
    std::string msg = std::string(zErrMsg);
    sqlite3_free(zErrMsg);
    throw Status{Status::LOGGER_CANNOT_INSERT,
                 std::string("Logger Err: ") + msg};
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
  std::string sql = std::string("CREATE TABLE IF NOT EXISTS ") +
                    this->tablename_ + std::string("(");
  sql += "ID INTEGER PRIMARY KEY";

  for (auto &field : fields) {
    table_map_[std::get<0>(field)] = std::get<1>(field);
    sql += std::string(", ") + std::string(std::get<0>(field)) +
           std::string(" ") + kSqlMapping[std::get<1>(field)];
  }
  sql +=
      ", Timestamp DATETIME DEFAULT (strftime('%Y-%m-%d %H:%M:%f', 'now', "
      "'localtime'))";

  sql += ");";

  /* Execute SQL statement */
  rc = sqlite3_exec(this->database_, sql.c_str(), nullptr, 0, &zErrMsg);

  if (rc) {
    std::string msg = std::string(zErrMsg);
    sqlite3_free(zErrMsg);
    sqlite3_close(this->database_);
    this->database_ = nullptr;
    throw Status{Status::LOGGER_CANNOT_OPEN, std::string("Logger Err: ") + msg};
  }
}

SQLiteLogger::~SQLiteLogger() {
  if (this->database_) {
    sqlite3_close(this->database_);
    this->database_ = nullptr;
  }
}

} /* namespace efimon */
