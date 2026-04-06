/**
 * @file csv-testing.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example of the use of the CSV logger
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <string>
#include <unordered_map>
#include <vector>

#include <efimon/logger/csv.hpp>

using namespace efimon;  // NOLINT

int main(int /*argc*/, char** /*argv*/) {
  static std::string filename{"logger.csv"};
  static std::string session{"MEASUREMENTS"};
  std::vector<Logger::MapTuple> table = {
      {"ProcessName", Logger::FieldType::STRING},
      {"PID", Logger::FieldType::INTEGER64},
  };

  /* Create the logger */
  CSVLogger logger{filename, table};

  /* Insert some values */
  std::unordered_map<std::string, std::shared_ptr<Logger::IValue>> values;

  values["ProcessName"] =
      std::make_shared<efimon::Logger::Value<std::string>>("notepad");
  values["PID"] = std::make_shared<efimon::Logger::Value<int64_t>>(2532);
  logger.InsertRow(values);

  values["ProcessName"] =
      std::make_shared<efimon::Logger::Value<std::string>>("gedit");
  values["PID"] = std::make_shared<efimon::Logger::Value<int64_t>>(2435);
  logger.InsertRow(values);

  return 0;
}
