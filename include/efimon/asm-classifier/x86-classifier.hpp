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
   */
  InstructionPair Classify(const std::string &inst) const noexcept override;

  /**
   * Default destructor for inheritance (implementation)
   */
  virtual ~x86Classifier() = default;
};

} /* namespace efimon */

#endif  // INCLUDE_EFIMON_ASM_CLASSIFIER_X86_CLASSIFIER_HPP_
