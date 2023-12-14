#include <iostream>
#include <memory>
#include <string>
#include <deque>

#include "Utilities.h"
#include "cache.h"
#include "cycle.h"
#include "emulator.h"

static Emulator* emulator = nullptr;
static Cache* iCache = nullptr;
static Cache* dCache = nullptr;
static std::string output;

static PipeState pipeState = {0,};
static uint32_t cycleCount;
static uint32_t icacheDelay = 0; // cycle delays for icache misses
static uint32_t dcacheDelay = 0; // cycle delays for dcache misses
static uint32_t stallDelay = 0; // cycle delays for stalls
static bool stalling = false;

static std::deque<int> memAddresses;

/**TODO: Implement the pipeline emulation for the MIPS machine in this section.
 * A basic template is provided below that doesn't account for all stalls.
 * Your task is to enhance it to correctly emulate the machine's running time, considering stalls.
 */

// initialize the emulator
Status initSimulator(CacheConfig& iCacheConfig, CacheConfig& dCacheConfig, MemoryStore* mem,
                     const std::string& output_name) {
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
        if (stallDelay > 0) {
            stallDelay--;
            stalling = true;
        }

        if (icacheDelay > 0) {
            icacheDelay--;
            stalling = true;
        }

        if (dcacheDelay > 0) {
            dcacheDelay--;
            stalling = true;
        }

        if (stalling) {
            stalling = false;

            // insert nop between USE operation and DEP operation
            // only if I-cache miss or Stall though
            // before: USE | DEP | A   | B   | C 
            // after:  USE | NOP | DEP | A   | B
            if (dcacheDelay == 0) {
                pipeState.cycle = cycleCount;
                pipeState.wbInstr = pipeState.memInstr;
                pipeState.memInstr = pipeState.exInstr;
                pipeState.exInstr = pipeState.idInstr;
                pipeState.idInstr = 0;
            }

            // cycle this as well
            memAddresses.push_front(-1);
            if (memAddresses.size() > 5) {
                memAddresses.pop_back();
            }

            count++;
            cycleCount++;
            continue;
        }

        Emulator::InstructionInfo info = emulator->executeInstruction();
        pipeState.cycle = cycleCount;  // get the execution cycle count

        //check if instruction hits overflow exception
        if(info.isOverflow){
            pipeState.wbInstr = pipeState.memInstr;
            pipeState.exInstr = 0;
            pipeState.idInstr = 0;
            pipeState.ifInstr = 0;
        }

        //check if invalid instruction and update pipeState
        if(!info.isValid){
            pipeState.wbInstr = pipeState.memInstr;
            pipeState.memInstr = pipeState.exInstr;
            pipeState.ifInstr = 0;
            pipeState.idInstr = 0;
        }

        // shuffle mem load/store addresses
        if (info.isValid && (info.opcode == OP_LBU || info.opcode == OP_LHU || info.opcode == OP_LW)) { // load
            memAddresses.push_front(info.loadAddress);
        }
        else if (info.isValid && (info.opcode == OP_SB || info.opcode == OP_SH || info.opcode == OP_SW)) { // store
            memAddresses.push_front(info.storeAddress);
        }
        else { // not load or store
            memAddresses.push_front(-1);
        }
        if (memAddresses.size() > 5) {
            memAddresses.pop_back();
        }

        // shuffle pipeline
        pipeState.wbInstr = pipeState.memInstr;
        pipeState.memInstr = pipeState.exInstr;
        pipeState.exInstr = pipeState.idInstr;
        pipeState.idInstr = pipeState.ifInstr;
        pipeState.ifInstr = info.instruction;

        // handle iCache delay
        icacheDelay += iCache->access(info.address, CACHE_READ) ? 0 : iCache->config.missLatency;

        // handle dCache delays (in a multicycle style)
        if (isLoad(pipeState.memInstr) && memAddresses.size() >= 4) {
            dcacheDelay += dCache->access(memAddresses[3], CACHE_READ) ? 0 : dCache->config.missLatency;
        }
        
        if (isStore(pipeState.memInstr) && memAddresses.size() >= 4) {
            dcacheDelay += dCache->access(memAddresses[3], CACHE_WRITE) ? 0 : dCache->config.missLatency;
        }
        

        /**
         * cases for stall delay:
         * 
         * load - op (ex. sub, add) = 1
         * load - store             = 0 (forwarded)
         * load - branch            = 2 (load obtains value after MEM, branch needs value during ID)
         * 
         * op - op                  = 0 (forwarded)
         * op - store               = 0 (lines up fine)
         * op - branch              = 1
         * 
         * check for dependency at each step
        */

       /**
        * If current instruction is branch (D dependency):
        *   If last instruction was op: insert 1
        *   If last instruction was load: insert 2
        * If current instruction is op (X dependency):
        *   If last instruction was load: insert 1
       */
        

        if (isBranch(pipeState.ifInstr))
        {
            if (isOp(pipeState.idInstr)) {
                // op-branch
                // note: all immediates use RT as target
                //           all r-type use RD as target
                //    branch operations use RS and RT

                // check if last op target is branch RS or branch RT

                uint32_t target;
                uint32_t br_a;
                uint32_t br_b;

                if (isImm(pipeState.idInstr)) {
                    target = rt(pipeState.idInstr);
                } else {
                    target = rd(pipeState.idInstr);
                }

                br_a = rs(pipeState.ifInstr);
                br_b = rt(pipeState.ifInstr);

                if (br_a == target || br_b == target) {
                    stallDelay = 1;
                }
            }

            if (isLoad(pipeState.idInstr)) {
                // load-branch
                // if branch uses the load target we must stall
                // load target is always rt
                uint32_t target = rt(pipeState.idInstr);
                uint32_t br_a;
                uint32_t br_b;

                br_a = rs(pipeState.ifInstr);
                br_b = rt(pipeState.ifInstr);

                if (br_a == target || br_b == target) {
                    stallDelay = 1;
                }
            }
        }

        if (isOp(pipeState.ifInstr)) {
            if (isLoad(pipeState.idInstr)) {
                // load-op
                // if op uses the load target we must stall
                // must discriminate between imm and r again (ugh)
                uint32_t target = rt(pipeState.idInstr);
                uint32_t op_a;
                uint32_t op_b;
                uint32_t imm = isImm(pipeState.ifInstr);

                // i-type operations use RS as argument
                // r-type operations use RS and RT as argument
                op_a = rs(pipeState.ifInstr);

                if (!imm) {
                    op_b = rt(pipeState.ifInstr);
                }

                // if current op uses results from previous 
                if ((imm && op_a == target) || (!imm && (op_a == target || op_b == target))) {
                    stallDelay = 1;
                }
            }
        }
        
        // increments count and global count at end of operations
        count++;
        cycleCount++;

        if (info.isHalt) {
            status = HALT;
            break;
        }
    }

    // Not exactly the right way, just as a demo here
    dumpPipeState(pipeState, output);
    return status;
}

// run till halt (call runCycles() with cycles == 1 each time) until
// status tells you to HALT or ERROR out
Status runTillHalt() {
    Status status;
    while (true) {
        status = static_cast<Status>(runCycles(1));
        if (status == HALT) break;
    }
    return status;
}

// dump the state of the emulator
Status finalizeSimulator() {
    emulator->dumpRegMem(output);
    SimulationStats stats{ emulator->getDin(), cycleCount, };  // TODO incomplete implementation
    dumpSimStats(stats, output);
    return SUCCESS;
}

uint32_t extract(uint32_t instruction, int start, int end) {
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

uint32_t funct(uint32_t target) {
  return extract(target, 5, 0);
}

// minus LUI because that's... immediate
bool isLoad(uint32_t target) {
  uint32_t op = opcode(target);
  return op == OP_LBU || op == OP_LHU || op == OP_LW;
}

bool isBranch(uint32_t target) {
  uint32_t op = opcode(target);
  return op == OP_BEQ || op == OP_BNE || op == OP_BLEZ || op == OP_BGTZ;
}

bool isStore(uint32_t target) {
  uint32_t op = opcode(target);
  return op == OP_SB || op == OP_SH || op == OP_SW;
}

bool isOp(uint32_t target) {
  return !isLoad(target) && !isBranch(target) && !isStore(target);
}

bool isImm(uint32_t target) {
  uint32_t op = opcode(target);
  uint32_t fun = funct(target);

  // note that all r-type functs are op
  // except JR (jump r)
  
  // is R type
  // is not Jump Register
  return isOp(target) && op == OP_ZERO && fun != FUN_JR;
}