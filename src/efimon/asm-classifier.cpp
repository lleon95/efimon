/**
 * @file asm-classifier.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief This header contains mappings and helpers to decypher the type
 * of ASM instruction executed within the system
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <efimon/asm-classifier.hpp>
#include <efimon/asm-classifier/x86-classifier.hpp>
#include <memory>
#include <string>

namespace efimon {

const std::string AsmClassifier::FamilyString(
    const assembly::InstructionFamily family) {
  switch (family) {
    case assembly::InstructionFamily::ARITHMETIC:
      return "Arithmetic";
    case assembly::InstructionFamily::LOGIC:
      return "Logic";
    case assembly::InstructionFamily::MEMORY:
      return "Memory";
    case assembly::InstructionFamily::BRANCH:
      return "Branch";
    case assembly::InstructionFamily::JUMP:
      return "Jump";
    default:
      return "Other";
  }
}

const std::string AsmClassifier::TypeString(
    const assembly::InstructionType type) {
  switch (type) {
    case assembly::InstructionType::SCALAR:
      return "Scalar";
    case assembly::InstructionType::VECTOR:
      return "Vector";
    default:
      return "Unclassified";
  }
}

std::unique_ptr<AsmClassifier> AsmClassifier::Build(
    const assembly::Architecture arch) {
  switch (arch) {
    case assembly::Architecture::X86:
      return std::make_unique<x86Classifier>();
    default:
      return nullptr;
  }
}

} /* namespace efimon */
