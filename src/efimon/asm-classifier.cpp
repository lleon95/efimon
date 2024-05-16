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

const std::string AsmClassifier::OriginString(const uint8_t origin) {
  uint8_t i = (origin >> static_cast<uint>(assembly::DataOrigin::INPUT)) &
              static_cast<uint>(assembly::DataOrigin::MASK);
  uint8_t o = (origin >> static_cast<uint>(assembly::DataOrigin::OUTPUT)) &
              static_cast<uint>(assembly::DataOrigin::MASK);

  auto orig = [](const uint8_t val) {
    switch (val) {
      case static_cast<uint8_t>(assembly::DataOrigin::MEMORY):
        return "mem";
      case static_cast<uint8_t>(assembly::DataOrigin::REGISTER):
        return "reg";
      case static_cast<uint8_t>(assembly::DataOrigin::IMMEDIATE):
        return "imm";
      default:
        return "unk";
    }
  };

  std::string s;
  if (origin == 0) {
    s = "unknown";
  } else {
    s = std::string(orig(i)) + std::string(":") + std::string(orig(o));
  }
  return s;
}

const std::pair<assembly::DataOrigin, assembly::DataOrigin>
AsmClassifier::OriginDecomposed(const uint8_t origin) {
  uint8_t i = (origin >> static_cast<uint>(assembly::DataOrigin::INPUT)) &
              static_cast<uint>(assembly::DataOrigin::MASK);
  uint8_t o = (origin >> static_cast<uint>(assembly::DataOrigin::OUTPUT)) &
              static_cast<uint>(assembly::DataOrigin::MASK);

  return {static_cast<assembly::DataOrigin>(i),
          static_cast<assembly::DataOrigin>(o)};
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
