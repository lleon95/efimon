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

const std::string x86Classifier::OperandTypes(const std::string &operands) const
    noexcept {
  std::string res = "";
  std::string firstop = "";
  std::string secondop = "";

  auto classify = [=](const std::string &in) {
    if (in.find("(") != std::string::npos) {
      return "m";
    } else if (in.find("$") != std::string::npos) {
      return "i";
    } else if (in.find("%") != std::string::npos) {
      return "r";
    } else {
      return "u";
    }
  };

  auto idx_firstmem = operands.find("),");
  auto idx_firstcomma = operands.find(",");
  if (idx_firstcomma == std::string::npos) {
    return "u";  // unique/none operand
  }

  /* Check if there is ),. Which means that the first operand is memory */
  if (idx_firstmem != std::string::npos) {
    res += "m";
    secondop = operands.substr(idx_firstmem + 2);
  } else {
    firstop = operands.substr(0, idx_firstcomma);
    secondop = operands.substr(idx_firstcomma + 1);
  }

  /* Check the other operands */
  if (!firstop.empty()) {
    res = classify(firstop) + res;
  }

  if (!secondop.empty()) {
    res = res + classify(secondop);
  }

  return res;
}

InstructionPair x86Classifier::Classify(const std::string &inst,
                                        const std::string &operands) const
    noexcept {
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
  uint8_t origin = 0;           /* Origin: OOII */

  if (inst.empty())
    return InstructionPair{InstructionType::UNCLASSIFIED,
                           InstructionFamily::OTHER, origin};

  /* Determine the origin */
  auto detorigin = [](const char in) {
    switch (in) {
      case 'r':
        return DataOrigin::REGISTER;
      case 'm':
        return DataOrigin::MEMORY;
      case 'i':
        return DataOrigin::IMMEDIATE;
      default:
        return DataOrigin::UNKNOWN;
    }
  };

  if (operands.size() == 2) {
    uint8_t o = static_cast<uint8_t>(detorigin(operands[0]));
    uint8_t i = static_cast<uint8_t>(detorigin(operands[1]));
    origin = (i << static_cast<uint8_t>(DataOrigin::INPUT)) |
             (o << static_cast<uint8_t>(DataOrigin::OUTPUT));
  } else if (operands.size() == 1) {
    origin = static_cast<uint8_t>(detorigin(operands[0]));
  }

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

  return InstructionPair{type, family, origin};
}

} /* namespace efimon */
