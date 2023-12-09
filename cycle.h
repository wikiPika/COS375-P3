#pragma once
#include <string>

#include "cache.h"
#include "Utilities.h"
#include "emulator.h"
#include "stdint.h"

// init the emulator and all info
Status initSimulator(CacheConfig& icConfig, CacheConfig& dcConfig, MemoryStore* memory,
                     const std::string& output_name);

// run the emulator for a certain number of cycles
Status runCycles(uint32_t cycles);

// run till halt (call runCycles() with cycles == 1 each time) until
// status tells you to HALT or ERROR out
Status runTillHalt();

// dump the state of the emulator
Status finalizeSimulator();

uint32_t extract(uint32_t target, int start, int end) {
  int bitsToExtract = start - end + 1;
  uint32_t mask = (1 << bitsToExtract) - 1;
  uint32_t clipped = instruction >> end;
  return clipped & mask;
}

uint32_t rs(uint32_t target) {
  return extract(target, 25, 21);
}

uint32_t rt(uint32_t target) {
  return extract(target, 20, 16);
}

uint32_t rd(uint32_t target) {
  return extract(target, 15, 11);
}

uint32_t opcode(uint32_t target) {
  return extract(target, 31, 26);
}

// minus LUI because that's... immediate
bool isLoad(uint32_t target) {
  uint32_t op = opcode(target);
  return op == OP_LBU || op == OP_LHU || op == OP_LW;
}

bool isBranch(uint32_t target) {
  uint32_t op = opcode(target);
  return op == OP_BEQ || op == OP_BNE || op == OP_BLEZ || op == OP_BGTZ
}

bool isOp(uint32_t target) {
  return !isLoad(target) && !isBranch(target);
}