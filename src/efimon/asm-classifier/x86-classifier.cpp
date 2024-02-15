/**
 * @file x86-classifier.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief This header contains mappings and helpers to decypher the type
 * of x86 ASM instruction executed within the system
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <algorithm>
#include <cctype>
#include <efimon/asm-classifier/x86-classifier.hpp>
#include <string>

namespace efimon {

InstructionPair x86Classifier::Classify(
    const std::string &inst) const noexcept {
  static const std::string kArithOp[] = {"add", "sub", "div",  "mul",
                                         "dp",  "abs", "sign", "avg",
                                         "dec", "inc", "neg"};
  static const std::string kBitManOp[] = {"shuf",     "lzcn",   "cvt",
                                          "blend",    "perm",   "extract",
                                          "compress", "insert", "unpck"};
  static const std::string kLogicOp[] = {"and",  "or",  "shl", "shr",
                                         "sll",  "sra", "srl", "tern",
                                         "test", "xor", "cmp", "not"};
  static const std::string kMemOp[] = {"expand", "gather", "scatter", "mov",
                                       "sto",    "lah",    "lds",     "lea",
                                       "les",    "lod"};
  static const std::string kJumpOp[] = {"jmp"};
  static const std::string kBranchOp[] = {"ja",  "jb", "jc", "je", "jg", "jl",
                                          "jle", "jn", "jo", "jp", "js", "jz"};

  using namespace assembly;  // NOLINT

  InstructionType type = InstructionType::UNCLASSIFIED; /* SCALAR, VECTOR... */
  InstructionFamily family =
      InstructionFamily::OTHER; /* ARITH, LOG, MEM, BRCH, JMP, OTHER */

  if (inst.empty())
    return InstructionPair{InstructionType::UNCLASSIFIED,
                           InstructionFamily::OTHER};

  /* Determine the family */
  auto pred = [&](const std::string &c) {
    return inst.find(c) != std::string::npos;
  };
  auto arthit = std::find_if(std::begin(kArithOp), std::end(kArithOp), pred);
  auto bitit = std::find_if(std::begin(kBitManOp), std::end(kBitManOp), pred);
  auto logit = std::find_if(std::begin(kLogicOp), std::end(kLogicOp), pred);
  auto memit = std::find_if(std::begin(kMemOp), std::end(kMemOp), pred);
  auto jmpit = std::find_if(std::begin(kJumpOp), std::end(kJumpOp), pred);
  auto branchit =
      std::find_if(std::begin(kBranchOp), std::end(kBranchOp), pred);

  if (arthit != std::end(kArithOp))
    family = InstructionFamily::ARITHMETIC;
  else if (bitit != std::end(kBitManOp))
    family = InstructionFamily::LOGIC;
  else if (logit != std::end(kLogicOp))
    family = InstructionFamily::LOGIC;
  else if (memit != std::end(kMemOp))
    family = InstructionFamily::MEMORY;
  else if (jmpit != std::end(kJumpOp))
    family = InstructionFamily::JUMP;
  else if (branchit != std::end(kBranchOp))
    family = InstructionFamily::BRANCH;

  bool compute_op = family == InstructionFamily::ARITHMETIC ||
                    family == InstructionFamily::LOGIC ||
                    family == InstructionFamily::MEMORY;

  /* Determine the type */
  switch (std::tolower(inst.at(0))) {
    case 'v':
      [[fallthrough]];
    case 'p':
      type =
          compute_op ? InstructionType::VECTOR : InstructionType::UNCLASSIFIED;
      break;
    default:
      type =
          compute_op ? InstructionType::SCALAR : InstructionType::UNCLASSIFIED;
      break;
  }

  return InstructionPair{type, family};
}

} /* namespace efimon */
