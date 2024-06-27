/**
 * @file ptx-classifier.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief This header contains mappings and helpers to decypher the type
 * of ptx ASM instruction executed within the system
 *
 * @copyright Copyright (c) 2024. See License for Licensing
 */

#include <algorithm>
#include <cctype>
#include <efimon/asm-classifier/ptx-classifier.hpp>
#include <string>

#include <iostream>

namespace efimon {

const std::string PtxClassifier::OperandTypes(const std::string &operands) const
    noexcept {
  std::string res = "";
  std::string pair = "";

  /* Trim spaces */
  std::string trimmed = operands;

  /* Get the first operand */
  bool is_vec = false;
  bool is_mem = false;
  bool is_reg = false;

  /* Operand reading */
  for (uint i = 0; i < trimmed.size(); ++i) {
    if (trimmed.at(i) == '{') {
      is_vec = true;
      res += 'v';
    } else if (trimmed.at(i) == '[') {
      is_mem = true;
      res += 'm';
    } else if (!is_vec && !is_mem && trimmed.at(i) == '%') {
      is_reg = true;
      res += 'r';
    } else if (!is_vec && !is_mem && !is_reg &&
               (trimmed.at(i) == ',' || trimmed.at(i) == ';')) {
      res += 'i';
    } else if (!is_vec && !is_mem && is_reg &&
               (trimmed.at(i) == ',' || trimmed.at(i) == ';')) {
      is_reg = false;
    } else if (trimmed.at(i) == '}' || trimmed.at(i) == ']') {
      is_vec = false;
      is_mem = false;
    }
  }

  /* Fill output operand */
  if (res.size() == 0) return "u";
  pair += res.at(0);

  auto to_weight = [](char t) {
    switch (t) {
      case 'i':
        return 1u;
      case 'r':
        return 2u;
      case 'v':
        return 3u;
      case 'm':
        return 4u;
      default:
        return 0u;
    }
  };

  /* Fill input operands */
  uint weight = 0u;
  char type = 'u';
  for (uint i = 1; i < res.size(); ++i) {
    auto w = to_weight(res[i]);
    if (weight < w) {
      weight = w;
      type = res[i];
    }
  }

  pair += type;

  // std::cout << "Operand Types: " << trimmed << " -> " << res << " p: " <<
  // pair << std::endl;

  // auto idx_firstmem = operands.find("],");
  // auto idx_firstvec = operands.find("},");
  // auto idx_firstcomma = operands.find(",");
  //
  return pair;
}

InstructionPair PtxClassifier::Classify(const std::string &inst,
                                        const std::string &operands) const
    noexcept {
  static const std::string kArithOp[] = {
      "add",   "sub", "div", "mul",  "mad",  "sad",  "rem", "abs",
      "neg",   "min", "max", "dp4a", "dp2a", "fma",  "rcp", "sqrt",
      "rsqrt", "sin", "cos", "lg2",  "ex2",  "tanh", ".mma"};
  static const std::string kBitManOp[] = {"popc",  "bfind", "fns",
                                          "brev",  "bef",   "bfi",
                                          "szext", "bmsk",  "copysign"};
  static const std::string kLogicOp[] = {
      "min", "max", "clz", "testp", "set",  "selp", "slct", "and.",
      "or.", "xor", "not", "cnot",  "lop3", "shf",  "shl",  "shr"};
  static const std::string kMemOp[] = {"mov",  "shfl",     "prmt", "ld.",
                                       "st.",  "prefetch", "cvt",  "replace",
                                       "load", "store"};
  static const std::string kJumpOp[] = {"call", "ret"};
  static const std::string kBranchOp[] = {"bra", "brx"};

  using namespace assembly;  // NOLINT

  InstructionType type = InstructionType::UNCLASSIFIED; /* SCALAR, VECTOR... */
  InstructionFamily family =
      InstructionFamily::OTHER; /* ARITH, LOG, MEM, BRCH, JMP, OTHER */
  uint8_t origin = 0;           /* Origin: OOII */

  if (inst.empty())
    return InstructionPair{InstructionType::UNCLASSIFIED,
                           InstructionFamily::OTHER, origin};

  /* Determine the origin */
  bool is_vector = false;
  auto detorigin = [&](const char in) {
    switch (in) {
      case 'v':
        is_vector = true;
        [[fallthrough]];
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

  // std::cout << "Instruction: " << inst << std::endl;

  /* Determine the type */
  switch (std::tolower(inst.at(0))) {
    case 'v':
      type =
          compute_op ? InstructionType::VECTOR : InstructionType::UNCLASSIFIED;
      break;
    default:
      type =
          compute_op ? InstructionType::SCALAR : InstructionType::UNCLASSIFIED;
      break;
  }
  if (inst.find("tensor") != std::string::npos ||
      inst.find("wmma") != std::string::npos ||
      inst.find("multi") != std::string::npos ||
      inst.find(".v") != std::string::npos || is_vector) {
    type = InstructionType::VECTOR;
  }

  return InstructionPair{type, family, origin};
}

} /* namespace efimon */
