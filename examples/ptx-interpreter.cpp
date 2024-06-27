/**
 * @file ptx-interpreter.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief Example the PTX Interpreter
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <efimon/asm-classifier/ptx-classifier.hpp>
#include <efimon/readings/instruction-readings.hpp>

int main(int argc, char **argv) {
  if (argc == 1) {
    std::cerr << "Error: Requires an argument with PTX" << std::endl;
    return -1;
  }

  std::string filepath(argv[1]);

  efimon::PtxClassifier classifier{};
  efimon::InstructionReadings readings{};
  std::ifstream file(filepath);

  if (!file.is_open()) {
    std::cerr << "Error: Cannot open the file: " << filepath << std::endl;
    return -1;
  }

  std::string line;
  bool found_entry = false;  // found the entry point
  bool reading = false;      // Ready to read the file (after finding the {})

  std::vector<efimon::InstructionPair> classifications{};
  std::vector<std::string> optypes{};

  while (std::getline(file, line)) {
    // Control flow
    if (line.find(".entry") == std::string::npos && !found_entry) {
      // Continue until finding the first function
      continue;
    } else if (line.find(".entry") != std::string::npos && !found_entry) {
      // Mark the first function as found
      std::cout << "INFO: Entry Found" << std::endl;
      found_entry = true;
      continue;
    } else if (line.find(".entry") != std::string::npos && found_entry) {
      // Break since we are starting with another function
      break;
    }

    // Look for the function body
    if (line[0] == '{') {
      // Prepare to start reading
      reading = true;
      continue;
    } else if (line[0] == '}') {
      // Abort the reading
      reading = false;
      break;
    }

    // If not reading, continue
    if (!reading || line.empty()) continue;

    // Trim spaces
    while (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
      line.erase(0, 1);
    }

    // Check emptiness
    if (line.empty()) continue;

    // Skip the labels and registers
    if (line[0] == '$') continue;
    if (line[0] == '.') continue;

    // Remove the predicates
    if (line[0] == '@') {
      auto it = line.find(" ");
      line = line.substr(it + 1);
    }

    // The line is ready at this point
    // std::cout << "Line: " << line << std::endl;

    // Load opcode and operands
    std::string assembly, operands, opcode;

    auto itspace = line.find(" ");
    auto itdot = line.find(".");
    auto itsecdot = itdot;
    if (itdot == std::string::npos) {
      itdot = itspace;
    } else {
      std::string secdot = line.substr(itdot + 1);
      itsecdot = line.find(".") + itdot + 1;
    }
    if (itsecdot == std::string::npos) itsecdot = itdot;
    assembly = line.substr(0, itspace);
    operands = line.substr(itspace + 1);
    opcode = line.substr(0, itsecdot);

    // Get the operand types
    std::string optype = classifier.OperandTypes(operands);
    efimon::InstructionPair classification =
        classifier.Classify(assembly, optype);

    optypes.push_back(opcode + "_" + optype);
    classifications.push_back(classification);
  }

  // Classify everything now
  float percent = 1.f / static_cast<float>(optypes.size());
  for (uint i = 0; i < optypes.size(); i++) {
    // Create histogram
    std::string assembly = optypes.at(i);
    if (readings.histogram.find(assembly) == readings.histogram.end()) {
      readings.histogram[assembly] = percent;
    } else {
      readings.histogram[assembly] += percent;
    }
    // Create the map
    efimon::InstructionPair classification = classifications.at(i);
    bool family_found =
        readings.classification[std::get<0>(classification)].find(
            std::get<1>(classification)) !=
        readings.classification[std::get<0>(classification)].end();
    if (!family_found) {
      readings.classification[std::get<0>(classification)]
                             [std::get<1>(classification)] = {};
    }
    bool origin_found = readings
                            .classification[std::get<0>(classification)]
                                           [std::get<1>(classification)]
                            .find(std::get<2>(classification)) !=
                        readings
                            .classification[std::get<0>(classification)]
                                           [std::get<1>(classification)]
                            .end();
    if (!origin_found) {
      readings.classification[std::get<0>(classification)][std::get<1>(
          classification)][std::get<2>(classification)] = 0.f;
    }
    readings.classification[std::get<0>(classification)][std::get<1>(
        classification)][std::get<2>(classification)] += percent;
  }

  // Print the histogram
  std::cout << "[HISTOGRAM]" << std::endl;
  for (const auto &pair : readings.histogram) {
    std::cout << "\t" << std::get<0>(pair) << ": " << std::get<1>(pair)
              << std::endl;
  }

  std::cout << "[TAXONOMY]" << std::endl;
  for (const auto &type : readings.classification) {
    std::cout << "\t" << efimon::AsmClassifier::TypeString(type.first) << ": "
              << std::endl;
    for (const auto &family : type.second) {
      std::cout << "\t\t" << efimon::AsmClassifier::FamilyString(family.first)
                << ": " << std::endl;
      for (const auto &origin : family.second) {
        std::cout << "\t\t\t"
                  << efimon::AsmClassifier::OriginString(origin.first) << ": "
                  << origin.second << std::endl;
      }
    }
  }

  return 0;
}
