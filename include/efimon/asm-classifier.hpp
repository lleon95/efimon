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
#include <tuple>
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

/**
 * Classifies the operands in different types
 *
 * It highlights if the operands come from registers or memory.
 * It is represented through bit shifting
 *
 * From the binary representation, it takes 4 bits: ooii, where
 * o stands for output representation and i stands for input
 */
enum class DataOrigin {
  /** Indicates that the operands do not have a explicit memory */
  UNKNOWN = 0b00,
  /** Indicates that data comes from memory */
  MEMORY = 0b01,
  /** Indicates that data comes from processor registers */
  REGISTER = 0b10,
  /** Indicates that data comes as immediate value */
  IMMEDIATE = 0b11,
  /** Indicates the input shift */
  INPUT = 0,
  /** Indicates the output shift */
  OUTPUT = 2,
  /** Sliding mask */
  MASK = 0b11,
};

}  // namespace assembly

/**
 * Type to return the type and family in a single run
 * The first type determines the instruction type, the second the family or
 * group and the third the data origin
 */
using InstructionPair =
    std::tuple<assembly::InstructionType, assembly::InstructionFamily, uint8_t>;

/**
 * Interface to classify the instructions into families and types
 */
class AsmClassifier {
 public:
  /**
   * Classifies the instruction from string to InstructionType and
   * InstructionFamily
   *
   * @param inst instruction
   * @param operands operands types
   * @return InstructionPair
   */
  virtual InstructionPair Classify(const std::string &inst,
                                   const std::string &operands) const
      noexcept = 0;

  /**
   * Determines if the operands belong to memory, immediate or register values
   *
   * @param operands as it comes from objdump
   * @return string with r, i or m symbolising the type of operands
   */
  virtual const std::string OperandTypes(const std::string &operands) const
      noexcept = 0;

  /**
   * Default destructor for inheritance (implementation)
   */
  virtual ~AsmClassifier() = default;

  /**
   * Gets the string from the family enumerator (InstructionFamily)
   * @param family family of instruction
   * @return instruction family in string
   */
  static const std::string FamilyString(
      const assembly::InstructionFamily family);

  /**
   * Gets the string from the type enumerator (InstructionType)
   * @param type type of instruction
   * @return instruction type in string
   */
  static const std::string TypeString(const assembly::InstructionType type);

  /**
   * Gets the string from the DataOrigin
   * @param origin origin of the data
   * @return instruction origin in string
   */
  static const std::string OriginString(const uint8_t origin);

  /**
   * Gets the decomposition from the DataOrigin
   * @param origin origin of the data
   * @return instruction origin in pair (input, output)
   */
  static const std::pair<assembly::DataOrigin, assembly::DataOrigin>
  OriginDecomposed(const uint8_t origin);

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
