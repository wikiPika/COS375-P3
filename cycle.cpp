#include "cycle.h"

#include <iostream>
#include <memory>
#include <string>

#include "Utilities.h"
#include "cache.h"
#include "emulator.h"

static Emulator* emulator = nullptr;
static Cache* iCache = nullptr;
static Cache* dCache = nullptr;
static std::string output;
static uint32_t cycleCount;

static uint32_t icacheDelay;
static uint32_t dcacheDelay;
static uint32_t stallDelay;

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
    PipeState pipeState = {0,};

    while (cycles == 0 || count < cycles) {
        Emulator::InstructionInfo info = emulator->executeInstruction();
        pipeState.cycle = cycleCount;  // get the execution cycle count
        
        // shuffle pipeline
        pipeState.wbInstr = pipeState.memInstr;
        pipeState.memInstr = pipeState.exInstr;
        pipeState.exInstr = pipeState.idInstr;
        pipeState.idInstr = pipeState.ifInstr;
        pipeState.ifInstr = info.instruction;

        uint32_t cacheDelay = 0;  // initially no delay for cache read/write
        uint32_t stallDelay = 0;  // for load-use

        // handle iCache delay
        cacheDelay += iCache->access(info.address, CACHE_READ) ? 0 : iCache->config.missLatency;

        // handle dCache delays (in a multicycle style)
        if (info.isValid && (info.opcode == OP_LBU || info.opcode == OP_LHU || info.opcode == OP_LW))
            cacheDelay += dCache->access(info.address, CACHE_READ) ? 0 : iCache->config.missLatency;

        if (info.isValid && (info.opcode == OP_SB || info.opcode == OP_SH || info.opcode == OP_SW))
            cacheDelay += dCache->access(info.address, CACHE_WRITE) ? 0 : iCache->config.missLatency;
        

        /**
         * cases for stall delay:
         * 
         * load - op (ex. sub, add) = 1
         * load - store             = 0 (forwarded)
         * load - branch            = 2 (load obtains value after MEM, branch needs value after ID)
         * 
         * op - op                  = 0 (forwarded)
         * op - store               = 0 (lines up fine)
         * op - branch              = 1
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
                stallDelay = 2;
            }

            if (isLoad(pipeState.idInstr)) {
                stallDelay = 1;
            }
        }
        if (isOp(pipeState.ifInstr)) {
            if (isLoad(pipeState.idInstr)) {
                stallDelay = 1;
            }
        }
        
        // increments din
        count += 1 + cacheDelay + stallDelay;
        cycleCount += 1 + cacheDelay + stallDelay;

        // if stall, send a (fes) nop(s) where it's "supposed" to be
        if (stallDelay > 0) {
            // reset stallDelay
            stallDelay = 0;

            // before
            // USE || DEP || A   || B   || C
            for (int i = 0; i < stallDelay; i++) {
                pipeState.wbInstr = pipeState.memInstr;
                pipeState.memInstr = pipeState.exInstr;
                pipeState.exInstr = pipeState.idInstr;
                pipeState.idInstr = 0;
            }
            
            // if single stall
            // if     id     x      mem    wb
            // USE || NOP || DEP || A   || B
            
            // or double stall
            // if     id     x      mem   wb
            // USE || NOP || NOP || DEP || A
        }

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
