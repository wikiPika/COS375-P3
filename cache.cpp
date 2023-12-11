// Sample cache implementation with random hit return

#include "cache.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
#include <cmath>

#include "Utilities.h"

using namespace std;

/** TODO[student] change this file to implement your cache */

// Random generator for cache hit/miss simulation
static std::mt19937 generator(42);  // Fixed seed for deterministic results
std::uniform_real_distribution<double> distribution(0.0, 1.0);

// Constructor definition
Cache::Cache(CacheConfig configParam, CacheDataType cacheType) : config(configParam), hits(0), misses(0) {
    numSets = config.cacheSize / config.ways / config.blockSize;

    tags.resize(numSets);
    for (auto &row : tags) {
        row.resize(config.ways);
    }

    order.resize(numSets);
    for (auto &row : order) {
        row.resize(config.ways);
    }

    valid.resize(numSets);
    for (auto &row : valid) {
        row.resize(config.ways, false);
    }




    // Here you can initialize other cache-specific attributes
    // For instance, if you had cache tables or other structures, initialize them here
}

// Counts Bits given power of two, n
int countBitsForPowerOfTwo(uint32_t n) {
    int count = 0;
    while (n > 1) {
        n = n >> 1;
        count++;
    }
    return count;
}

// Access method definition
bool Cache::access(uint32_t address, CacheOperation readWrite) {
    uint32_t numBlockOffsetBits = countBitsForPowerOfTwo(config.blockSize);
    uint32_t numIndexBits = countBitsForPowerOfTwo(numSets);
    // uint32_t numTagBits = 32 - numBlockOffsetBits - numIndexBits;

    // uint32_t blockOffsetMask = (1 << numBlockOffsetBits) - 1;
    uint32_t indexMask = (1 << numIndexBits) - 1;

    // uint32_t blockOffset = address & blockOffsetMask;
    uint32_t index = (address >> numBlockOffsetBits) & indexMask;
    uint32_t tag = address >> (numBlockOffsetBits + numIndexBits);


    bool hit = false;

    // if hit
    for (uint32_t i = 0; i < config.ways; i++) {
        if (valid[index][i] && tags[index][i] == tag) {
            hit = true;
            // update order
            for (uint32_t j = 0; j < config.ways; j++) {
                if (valid[index][j] && order[index][j] < order[index][i]) {
                    order[index][j]++;
                }
            }
            order[index][i] = 1;
        }
    }
    // if miss
    if (hit == false) {
        uint32_t numValid = 0;
        for (uint32_t i = 0; i < config.ways; i++) {
            if (valid[index][i]) {
                numValid++;
            }
        }

        // if do not need to evict
        if (numValid < config.ways) {
            for (uint32_t i = 0; i < config.ways; i++) {
                if (valid[index][i]) {
                    order[index][i]++;
                }
            }
            for (uint32_t i = 0; i < config.ways; i++) {
                if (!valid[index][i]) {
                    tags[index][i] = tag;
                    order[index][i] = 1;
                    valid[index][i] = true;
                    break;
                }
            }
        }
        else
        {
            for (uint32_t i = 0; i < config.ways; i++) {
                if (order[index][i] == config.ways) {
                    tags[index][i] = tag;
                }
            }
            // reorder
            for (uint32_t j = 0; j < config.ways; j++) {
                order[index][j] = order[index][j] % config.ways + 1;
            }
        }
    }



    // For simplicity, we're using a random boolean to simulate cache hit/miss
    // bool hit = distribution(generator) < 0.20;  // random 20% hit for a strange cache
    hits += hit;
    misses += !hit;
    return hit;
}

// Dump method definition, you can write your own dump info
Status Cache::dump(const std::string& base_output_name) {
    ofstream cache_out(base_output_name + "_cache_state.out");
    // dumpRegisterStateInternal(reg, cache_out);
    if (cache_out) {
        cache_out << "---------------------" << endl;
        cache_out << "Begin Register Values" << endl;
        cache_out << "---------------------" << endl;
        cache_out << "Cache Configuration:" << std::endl;
        cache_out << "Size: " << config.cacheSize << " bytes" << std::endl;
        cache_out << "Block Size: " << config.blockSize << " bytes" << std::endl;
        cache_out << "Ways: " << (config.ways == 1) << std::endl;
        cache_out << "Miss Latency: " << config.missLatency << " cycles" << std::endl;
        cache_out << "---------------------" << endl;
        cache_out << "End Register Values" << endl;
        cache_out << "---------------------" << endl;
        return SUCCESS;
    } else {
        cerr << LOG_ERROR << "Could not create cache state dump file" << endl;
        return ERROR;
    }

    // Here you can also dump the state of the cache, its stats, or any other relevant information
}
