#include "MemoryStore.h"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "Utilities.h"

using namespace std;

MemoryStore::MemoryStore(uint32_t startAddr, uint32_t numEntries)
    : startAddr(startAddr), numEntries(numEntries) {
    memArr = new uint8_t[numEntries];

    for (uint32_t i = 0; i < numEntries; i++) {
        memArr[i] = 0;
    }
    // If we can't initialise memory appropriately, don't return a
    // MemoryStore at all.
    assert((prepareMemory(this) == 0));
}

MemoryStore::MemoryStore(uint32_t startAddr, uint32_t numEntries, const char *fileName)
    : startAddr(startAddr), numEntries(numEntries) {
    memArr = new uint8_t[numEntries];

    for (uint32_t i = 0; i < numEntries; i++) {
        memArr[i] = 0;
    }
    // If we can't initialise memory appropriately, don't return a
    // MemoryStore at all.
    assert((prepareMemory(this) == 0));

    loadFromFile(fileName);
}

int prepareMemory(MemoryStore *mem) {
    ifstream initMem;
    initMem.open("init_mem_image", ios::in);

    // For tests that don't require such an initial memory image, nothing is done.
    while (initMem && mem) {
        uint32_t curVal = 0;
        uint32_t addr = 0;

        initMem >> hex >> addr;
        initMem >> hex >> curVal;

        int ret = mem->setMemValue(addr, curVal, WORD_SIZE);

        if (ret) {
            cout << LOG_ERROR << "Could not set initial memory value!" << endl;
            return -EINVAL;
        }
    }

    return 0;
}

int MemoryStore::getOrSetValue(bool get, uint32_t address, uint32_t &value, MemEntrySize size) {
    uint32_t byteSize = (uint32_t)(size);

    if (address < startAddr || address + byteSize >= (startAddr + numEntries)) {
        cerr << LOG_ERROR << "Address 0x" << hex << address << " is out of range" << endl;
        return -EINVAL;
    }

    switch (size) {
        case BYTE_SIZE:
        case HALF_SIZE:
        case WORD_SIZE:
            break;
        default:
            cerr << LOG_ERROR << "Invalid size passed, cannot read/write memory" << endl;
            return -EINVAL;
    }

    uint32_t relativeAddr = address - startAddr;
    uint8_t *valPtr = memArr + relativeAddr;

    if (get) {
        value = 0;

        // The values are stored and loaded byte by byte to combat x86 being little-endian.
        while (byteSize > 0) {
            value |= (*valPtr << ((byteSize - 1) * BYTE_SHIFT));
            valPtr++;
            byteSize--;
        }
    } else {
        while (byteSize > 0) {
            *valPtr = (value >> ((byteSize - 1) * 8)) & 0xFF;
            valPtr++;
            byteSize--;
        }
    }

    return 0;
}

int MemoryStore::getMemValue(uint32_t address, uint32_t &value, MemEntrySize size) {
    return getOrSetValue(true, address, value, size);
}

int MemoryStore::setMemValue(uint32_t address, uint32_t value, MemEntrySize size) {
    return getOrSetValue(false, address, value, size);
}

int MemoryStore::loadFromFile(const char *fileName) {
    // Open instruction file
    ifstream infile(fileName, ios::binary | ios::in);

    if (infile) {
        // Get length of the file and read instruction file into a buffer
        infile.seekg(0, ios::end);
        int length = infile.tellg();
        infile.seekg(0, ios::beg);

        char *buf = new char[length];
        infile.read(buf, length);
        infile.close();

        // Initialize memory store with buffer contents
        int memLength = length / sizeof(buf[0]);
        int i;
        for (i = 0; i < memLength; i++) {
            this->setMemValue(i * BYTE_SIZE, buf[i], BYTE_SIZE);
        }
        return SUCCESS;
    } else {
        std::cerr << LOG_ERROR << "Unable to open memory file " << fileName << std::endl;
        return ERROR;
    }
}

int MemoryStore::printMemArray(uint32_t startAddr, uint32_t endAddr, uint32_t entrySize,
                               uint32_t entriesPerRow, std::ostream &out_stream) {
    // Note that here endAddr isn't printed.
    if (startAddr > endAddr || startAddr < startAddr || endAddr > (startAddr + numEntries)) {
        cerr << LOG_ERROR << "Address range 0x" << hex << startAddr << "-0x" << endAddr
             << " is out of range" << endl;
        return -EINVAL;
    }

    switch (entrySize) {
        case BYTE_SIZE:
        case HALF_SIZE:
        case WORD_SIZE:
            break;
        default:
            cerr << LOG_ERROR << "Invalid print size passed, cannot print memory" << endl;
            return -EINVAL;
    }

    uint32_t relStart = startAddr - startAddr;
    uint32_t relEnd = endAddr - startAddr;
    uint32_t curAddr = startAddr;

    uint8_t *valPtr = memArr + relStart;
    uint8_t *endPtr = memArr + relEnd;

    while (valPtr < endPtr) {
        out_stream << "0x" << hex << setfill('0') << setw(WORD_WIDTH) << curAddr << ": ";
        for (uint32_t i = 0; i < entriesPerRow; i++) {
            if (valPtr < endPtr) {
                out_stream << "0x";
                for (int i = 0; i < (int)(entrySize); i++) {
                    out_stream << hex << setfill('0') << setw(BYTE_WIDTH) << (uint32_t)(*valPtr);
                    valPtr++;
                }
                out_stream << " ";
            } else {
                // We're done before ending the row.
                out_stream << endl;
                return 0;
            }
        }

        out_stream << endl;
        curAddr += (uint32_t)(entrySize)*entriesPerRow;
    }

    return 0;
}

int MemoryStore::printMemory(uint32_t startAddr, uint32_t endAddress) {
    return printMemArray(startAddr, endAddress, WORD_SIZE, 5, std::cout);
}

void dumpMemoryState(MemoryStore *mem, const std::string &base_output_name) {
    uint32_t startAddr;
    uint32_t endAddr;
    startAddr = 0;
    endAddr = 0x1f4;  // 500

    MemoryStore *memImpl = dynamic_cast<MemoryStore *>(mem);
    if (!memImpl) {
        cerr << LOG_ERROR << "Invalid memory store passed to dump function" << endl;
    }
    ifstream memRange;
    memRange.open("print_mem_range", ios::in);
    // For tests that don't specify a memory range to print out, the default is used.
    if (memRange) {
        memRange >> hex >> startAddr;
        memRange >> hex >> endAddr;
    }
    ofstream mem_out(base_output_name + "_mem_state.out");

    if (mem_out) {
        mem_out << "---------------------" << endl;
        mem_out << "Begin Memory State" << endl;
        mem_out << "---------------------" << endl;
        memImpl->printMemArray(startAddr, endAddr, WORD_SIZE, 5, mem_out);
        mem_out << "---------------------" << endl;
        mem_out << "End Memory State" << endl;
        mem_out << "---------------------" << endl;
    } else {
        cerr << LOG_ERROR << "Could not create memory state dump file" << endl;
    }
}
