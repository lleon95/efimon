/**
 * @file sqlite-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of the use of the SQLite logger
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <string>
#include <unordered_map>
#include <vector>

#include <efimon/logger/sqlite.hpp>

using namespace efimon;  // NOLINT

int main(int /*argc*/, char** /*argv*/) {
  static std::string filename{"logger.db"};
  static std::string session{"MEASUREMENTS"};
  std::vector<Logger::MapTuple> table = {
      {"ProcessName", Logger::FieldType::STRING},
      {"PID", Logger::FieldType::INTEGER64},
  };

  /* Create the logger */
  SQLiteLogger logger{filename, session, table};

  /* Insert some values */
  std::unordered_map<std::string, std::shared_ptr<Logger::IValue>> values;
  values["ProcessName"] =
      std::make_shared<efimon::Logger::Value<std::string>>("notepad");
  values["PID"] = std::make_shared<efimon::Logger::Value<int64_t>>(2532);
  logger.InsertRow(values);

  return 0;
}
