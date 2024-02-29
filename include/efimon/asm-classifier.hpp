/**
 * @file asm-classifier.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief This header contains mappings and helpers to decypher the type
 * of ASM instruction executed within the system
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_ASM_CLASSIFIER_HPP_
#define INCLUDE_EFIMON_ASM_CLASSIFIER_HPP_

#include <memory>
#include <string>
#include <utility>

namespace efimon {
namespace assembly {

/**
 * Enumerator for the architecture
 */
enum class Architecture {
  /** Unknown */
  NONE = 0,
  /** x86 architecture */
  X86
};

/**
 * Enumerator for the instruction family, classifying it according to its
 * functionality
 */
enum class InstructionFamily {
  /** Arithmetic: add, sub, div, mul */
  ARITHMETIC = 0,
  /** Logic: and, or, not, xor, shift, mask */
  LOGIC,
  /** Memory: mov, load, store */
  MEMORY,
  /** Conditional branching: test, jz.. */
  BRANCH,
  /** Unconditional branching: jmp, jump... */
  JUMP,
  /** Other not absorbed above*/
  OTHER
};

/**
 * Enumerator for the instruction type, classifying according to its SIMDness
 */
enum class InstructionType {
  /** Scalar instructions: without any kind of vectorisation */
  SCALAR = 0,
  /** Vector instructions: including matrix and vector */
  VECTOR,
  /** Unclassified instructions, like stacking pop and push */
  UNCLASSIFIED
};

}  // namespace assembly

/**
 * Type to return the type and family in a single run
 */
using InstructionPair =
    std::pair<assembly::InstructionType, assembly::InstructionFamily>;

/**
 * Interface to classify the instructions into families and types
 */
class AsmClassifier {
 public:
  /**
   * Classifies the instruction from string to InstructionType and
   * InstructionFamily
   */
  virtual InstructionPair Classify(const std::string &inst) const noexcept = 0;

  /**
   * Default destructor for inheritance (implementation)
   */
  virtual ~AsmClassifier() = default;

  /**
   * Gets the string from the family enumerator (InstructionFamily)
   */
  static const std::string FamilyString(
      const assembly::InstructionFamily family);

  /**
   * Gets the string from the type enumerator (InstructionType)
   */
  static const std::string TypeString(const assembly::InstructionType type);

  /**
   * @brief Constructs a new classifier.
   *
   * @param arch architecture selector from assembly::Architecture enum
   * @return unique_ptr to the classifier implementation
   */
  static std::unique_ptr<AsmClassifier> Build(
      const assembly::Architecture arch);
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_ASM_CLASSIFIER_HPP_
