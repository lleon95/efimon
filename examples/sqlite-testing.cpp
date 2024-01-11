/**
 * @file sqlite-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of the use of the SQLite logger
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/logger/sqlite.hpp>

using namespace efimon;  // NOLINT

int main(int /*argc*/, char** /*argv*/) {
  static std::string filename{"logger.db"};
  static std::string session{"MEASUREMENTS"};
  std::vector<Logger::MapTuple> table = {
      {"ProcessName", Logger::FieldType::STRING},
      {"PID", Logger::FieldType::INTEGER},
  };

  SQLiteLogger logger{filename, session, table};

  return 0;
}
