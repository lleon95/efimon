/**
 * @file x86-classifier.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief This header contains mappings and helpers to decypher the type
 * of x86 ASM instruction executed within the system
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_ASM_CLASSIFIER_X86_CLASSIFIER_HPP_
#define INCLUDE_EFIMON_ASM_CLASSIFIER_X86_CLASSIFIER_HPP_

#include <efimon/asm-classifier.hpp>
#include <string>

namespace efimon {

/**
 * Interface to classify the x86 instructions into families and types
 */
class x86Classifier : public AsmClassifier {
 public:
  /**
   * Classifies the instruction from string to InstructionType and
   * InstructionFamily
   *
   * @param inst instruction
   * @param operands operands types
   * @return InstructionPair
   */
  InstructionPair Classify(const std::string &inst,
                           const std::string &operands) const noexcept override;

  /**
   * Determines if the operands belong to memory, immediate or register values
   *
   * @param operands as it comes from objdump
   * @return string with r, i or m symbolising the type of operands
   */
  const std::string OperandTypes(const std::string &operands) const
      noexcept override;

  /**
   * Default destructor for inheritance (implementation)
   */
  virtual ~x86Classifier() = default;
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_ASM_CLASSIFIER_X86_CLASSIFIER_HPP_
