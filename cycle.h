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

uint32_t extract(uint32_t instruction, int start, int end);
uint32_t rs(uint32_t target);
uint32_t rt(uint32_t target);
uint32_t rd(uint32_t target);
uint32_t opcode(uint32_t target);
uint32_t funct(uint32_t target);

// minus LUI because that's... immediate
bool isLoad(uint32_t target);
bool isBranch(uint32_t target);
bool isStore(uint32_t target);
bool isOp(uint32_t target);
bool isImm(uint32_t target);