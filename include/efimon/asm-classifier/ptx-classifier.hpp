/**
 * @file ptx-classifier.hpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief This header contains mappings and helpers to decypher the type
 * of ptx ASM instruction executed within the system
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#ifndef INCLUDE_EFIMON_ASM_CLASSIFIER_PTX_CLASSIFIER_HPP_
#define INCLUDE_EFIMON_ASM_CLASSIFIER_PTX_CLASSIFIER_HPP_

#include <efimon/asm-classifier.hpp>
#include <string>

namespace efimon {

/**
 * Interface to classify the ptx instructions into families and types
 */
class PtxClassifier : public AsmClassifier {
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
  virtual ~PtxClassifier() = default;
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_ASM_CLASSIFIER_PTX_CLASSIFIER_HPP_
