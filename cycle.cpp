#include <array>
#include <iostream>
#include <memory>
#include <string>

#include "Utilities.h"
#include "cache.h"
#include "cycle.h"
#include "emulator.h"

static Emulator *emulator = nullptr;
static Cache *iCache = nullptr;
static Cache *dCache = nullptr;
static std::string output;

static PipeState pipeState = {
    0,
};
static uint32_t cycleCount;
static uint32_t iMiss = 0;  // cycle delays for icache misses
static uint32_t dMiss = 0;  // cycle delays for dcache misses
static uint32_t dStall = 0; // load-branches (they insert at d)
static uint32_t xStall = 0; // load-op and op-branch (they insert at x)
static uint32_t except = 0; // for exceptions, duh.

static std::array<int, 5> memAddresses{0, 0, 0, 0, 0}; // for proper load/store tracking

/**
 * Cycle behavior entirely implemented by yours truly, Jackie Liu <3
 */
Status initSimulator(CacheConfig &iCacheConfig, CacheConfig &dCacheConfig,
                     MemoryStore *mem, const std::string &output_name) {
  output = output_name;
  emulator = new Emulator();
  emulator->setMemory(mem);
  iCache = new Cache(iCacheConfig, I_CACHE);
  dCache = new Cache(dCacheConfig, D_CACHE);
  return SUCCESS;
}

// run the emulator for a certain number of cycles
// return SUCCESS if reaching desired cycles.
// return HALT if the simulator halts on 0xfeedfeed
Status runCycles(uint32_t cycles) {
  uint32_t count = 0;
  auto status = SUCCESS;

  while (cycles == 0 || count < cycles) {
    /**
     * Exceptions are really, really fucking annoying on this machine.
     * Because of the sequential nature, even though an exception happens,
     * You have to fake what the pipeline does. Just great!
     */

    pipeState.cycle = cycleCount;

    if (except > 0) {
      except--;

      ingestPipeline(0);
      ingestBuffer(-1);

      count++;
      continue;
    }

    /**
     * If stalling or delaying, decrease and keep progressing.
     */

    if (iMiss > 0 || dMiss > 0 || dStall > 0 || xStall > 0) {
      if (dMiss == 0) {
        if (xStall > 0) {
          pipeState.wbInstr = pipeState.memInstr;
          pipeState.memInstr = pipeState.exInstr;
          pipeState.exInstr = 0;

          memAddresses[4] = memAddresses[3];
          memAddresses[3] = memAddresses[2];
          memAddresses[2] = -1;
        } else if (dStall > 0 || iMiss > 0) {
          pipeState.wbInstr = pipeState.memInstr;
          pipeState.memInstr = pipeState.exInstr;
          pipeState.exInstr = pipeState.idInstr;
          pipeState.idInstr = 0;

          memAddresses[4] = memAddresses[3];
          memAddresses[3] = memAddresses[2];
          memAddresses[2] = memAddresses[1];
          memAddresses[1] = -1;
        }
      } else {
        // can safely wipe this (exits pipeline)
        pipeState.wbInstr = 0;
      }

      if (iMiss > 0)
        iMiss--;
      if (dMiss > 0)
        dMiss--;
      if (dStall > 0)
        dStall--;
      if (xStall > 0)
        xStall--;

      if (iMiss > 0 || dStall > 0 || xStall > 0) {
        // we check data here as well ?
        if (isLoad(pipeState.memInstr) && memAddresses[3] != -1) {
          dMiss += dCache->access(memAddresses[3], CACHE_READ)
                       ? 0
                       : dCache->config.missLatency;
        }

        if (isStore(pipeState.memInstr) && memAddresses[3] != -1) {
          dMiss += dCache->access(memAddresses[3], CACHE_WRITE)
                       ? 0
                       : dCache->config.missLatency;
        }

        if (dMiss > 0) {
          println("d-cache miss in stall");
        }
      }

      count++;
      continue;
    }

    /**
     * Normal operation.
     */

    Emulator::InstructionInfo info = emulator->executeInstruction();

    // Ingest new instruction into the pipeline
    if (info.isOverflow) {

      // If overflow, zero current and next two instructions
      except = 2;
      ingestPipeline(0);
      ingestBuffer(-1);
      println("arithmetic error");

      count++;
      continue;
    } else if (!info.isValid) {

      // If invalid, zero current and next instruction
      except = 1;
      ingestPipeline(0);
      ingestBuffer(-1);
      println("illegal");

      count++;
      continue;
    } else

      // Else ingest the instruction as normal
      ingestPipeline(info.instruction);

    // Keep track of memory accesses for cache
    // note: only valid instructions will pass this stage
    ingestBuffer(isLoad(info.instruction)    ? info.loadAddress
                 : isStore(info.instruction) ? info.storeAddress
                                             : -1);

    /**
     * Cache delays
     *
     * For iCache, that's just what it is lmao
     * for dCache, we probably have to look ahead a bit
     */

    iMiss =
        iCache->access(info.pc, CACHE_READ) ? 0 : iCache->config.missLatency;

    if (isLoad(pipeState.memInstr) && memAddresses[3] != -1) {
      dMiss += dCache->access(memAddresses[3], CACHE_READ)
                   ? 0
                   : dCache->config.missLatency;
    }

    if (isStore(pipeState.memInstr) && memAddresses[3] != -1) {
      dMiss += dCache->access(memAddresses[3], CACHE_WRITE)
                   ? 0
                   : dCache->config.missLatency;
    }

    if (iMiss > 0) {
      println("i-cache miss");
    }

    if (dMiss > 0) {
      println("d-cache miss");
    }

    /**
     * Stall delays
     * 
     * Four main types:
     * 
     * op-branch          = 1 (D, X)
     * load-op            = 1 (D, X)
     * load-branch        = 2 (F, D)
     * load-(smth)-branch = 1 (F, X)
     */

    uint32_t fin = pipeState.ifInstr;
    uint32_t din = pipeState.idInstr;
    uint32_t xin = pipeState.exInstr;

    xStall = op_branch(din, xin) + load_op(din, xin);

    if (load_branch(fin, din) == load_branch(fin, xin) &&
        load_branch(fin, din) == 2) {
      // load-branch overrides load-something-branch
      println("load-branch detected");
      dStall = 2;
    } else if (load_branch(fin, din) == 2 || load_branch(fin, xin) == 2) {
      // if load - something - branch, only do 1
      dStall = load_branch(fin, din) + (load_branch(fin, xin) / 2);
      if (dStall == 1) {
        println("load-smth-branch detected");
      } else {
        println("load-branch detected");
      }
    }

    if (info.isHalt) {
      status = HALT;
      // flush everything
      for (int i = 0; i < 4; i++) {
        // shuffle pipeline
        dump();
        cycleCount++;
        pipeState.cycle = cycleCount;

        ingestPipeline(0);
        ingestBuffer(-1);
      }

      break;
    }

    /**
     * Very end of loop
     */

    count++;
  }

  printBuffer();
  printCycle();
  dump();
  cycleCount++;
  return status;
}

void dump() { dumpPipeState(pipeState, output); }

void printBuffer() {
  cout << "buffer: " << memAddresses[0] << " | " << memAddresses[1] << " | "
       << memAddresses[2] << " | " << memAddresses[3] << " | "
       << memAddresses[4] << endl;
}

void printCycle() {
  cout << "cycle: " << cycleCount << " (" << dStall << " | " << xStall << " | "
       << iMiss << " | " << dMiss << ") " << endl
       << endl;
}

void println(string x) { cout << x << endl; }

int op_branch(uint32_t use, uint32_t dep) {
  if (isBranch(use) && isOp(dep)) {
    uint32_t target;
    uint32_t br_a;
    uint32_t br_b;

    if (isImm(dep)) {
      target = rt(dep);
    } else {
      target = rd(dep);
    }

    br_a = rs(use);
    br_b = rt(use);

    if (br_a == target || br_b == target) {
      println("op-branch detected");
      cout << br_a << " | " << br_b << " = " << target << endl;
      return 1;
    }
  }

  return 0;
}

int load_branch(uint32_t use, uint32_t dep) {
  if (isBranch(use) && isLoad(dep)) {
    uint32_t target = rt(dep);
    uint32_t br_a;
    uint32_t br_b;

    br_a = rs(use);
    br_b = rt(use);

    if (br_a == target || br_b == target) {
      cout << br_a << " | " << br_b << " = " << target << endl;
      return 2;
    }
  }

  return 0;
}
int load_op(uint32_t use, uint32_t dep) {
  if (isOp(use) && isLoad(dep)) {
    uint32_t target = rt(dep);
    uint32_t op_a;
    uint32_t op_b = -1;
    uint32_t imm = isImm(use);

    op_a = rs(use);

    if (!imm) {
      op_b = rt(use);
    }

    if ((imm && op_a == target) ||
        (!imm && (op_a == target || op_b == target))) {
      println("load-op detected");
      cout << op_a << " | " << op_b << " = " << target << endl;
      return 1;
    }
  }

  return 0;
}

void ingestBuffer(uint32_t in) {
  memAddresses[4] = memAddresses[3];
  memAddresses[3] = memAddresses[2];
  memAddresses[2] = memAddresses[1];
  memAddresses[1] = memAddresses[0];
  memAddresses[0] = in;
}

void ingestPipeline(uint32_t in) {
  pipeState.cycle = cycleCount;
  pipeState.wbInstr = pipeState.memInstr;
  pipeState.memInstr = pipeState.exInstr;
  pipeState.exInstr = pipeState.idInstr;
  pipeState.idInstr = pipeState.ifInstr;
  pipeState.ifInstr = in;
}

// run till halt (call runCycles() with cycles == 1 each time) until
// status tells you to HALT or ERROR out
Status runTillHalt() {
  Status status;
  while (true) {
    status = static_cast<Status>(runCycles(1));
    if (status == HALT)
      break;
  }
  return status;
}

// dump the state of the emulator
Status finalizeSimulator() {
  emulator->dumpRegMem(output);
  SimulationStats stats{
      emulator->getDin(),
      cycleCount,
  }; // TODO incomplete implementation
  dumpSimStats(stats, output);
  return SUCCESS;
}

uint32_t extract(uint32_t instruction, int start, int end) {
  int bitsToExtract = start - end + 1;
  uint32_t mask = (1 << bitsToExtract) - 1;
  uint32_t clipped = instruction >> end;
  return clipped & mask;
}

uint32_t rs(uint32_t target) { return extract(target, 25, 21); }

uint32_t rt(uint32_t target) { return extract(target, 20, 16); }

uint32_t rd(uint32_t target) { return extract(target, 15, 11); }

uint32_t opcode(uint32_t target) { return extract(target, 31, 26); }

uint32_t funct(uint32_t target) { return extract(target, 5, 0); }

// minus LUI because that's... immediate
bool isLoad(uint32_t target) {
  if (target == 0)
    return false;
  uint32_t op = opcode(target);
  return op == OP_LBU || op == OP_LHU || op == OP_LW;
}

bool isBranch(uint32_t target) {
  if (target == 0)
    return false;
  uint32_t op = opcode(target);
  return op == OP_BEQ || op == OP_BNE || op == OP_BLEZ || op == OP_BGTZ;
}

bool isStore(uint32_t target) {
  if (target == 0)
    return false;
  uint32_t op = opcode(target);
  return op == OP_SB || op == OP_SH || op == OP_SW;
}

bool isOp(uint32_t target) {
  if (target == 0)
    return false;
  return isImm(target) || isRop(target);
}

bool isRop(uint32_t target) {
  if (target == 0)
    return false;

  uint32_t op = opcode(target);
  uint32_t fun = funct(target);

  // note that all r-type functs are op
  // except JR (jump r)

  // is R type
  // is not Jump Register
  return op == OP_ZERO && fun != FUN_JR;
}

bool isImm(uint32_t target) {
  uint32_t op = opcode(target);

  return op == OP_ADDI || op == OP_ADDIU || op == OP_ANDI || op == OP_ORI ||
         op == OP_SLTI || op == OP_SLTIU;
}