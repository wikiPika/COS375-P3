#include <iostream>
#include <vector>

#include "cache.h"

using namespace std;

void test_cache(int test_num, uint32_t cacheSize, uint32_t blockSize, uint32_t ways, int hits, int misses, std::vector<uint32_t> test_addresses) {
    CacheConfig cc = {cacheSize, blockSize, ways, 5};
    CacheDataType ctype = {I_CACHE};
    CacheOperation cor = {CACHE_READ};
    CacheOperation cow = {CACHE_WRITE};
    {
        Cache cache = Cache(cc, ctype);

        for (uint32_t address : test_addresses) {
            cache.access(address, cor);
        }
        bool test = cache.getHits() == hits && cache.getMisses() == misses;
        cout << "Test " << test_num << " Read: ";
        if (test) {
            cout << "Passed" << endl;
        } else {
            cout << "Failed" << endl;
        }
    }
    {
        Cache cache = Cache(cc, ctype);

        for (uint32_t address : test_addresses) {
            cache.access(address, cow);
        }
        bool test = cache.getHits() == hits && cache.getMisses() == misses;
        cout << "Test " << test_num << " Write: ";
        if (test) {
            cout << "Passed" << endl;
        } else {
            cout << "Failed" << endl;
        }
    }
}

int main(int argc, char** argv) {
    // Tests also check that writes have the exact same behavior

    // Test one cache line being used for multiple words
    vector<uint32_t> test1 = {0b000, 0b100};
    test_cache(1, 8, 8, 1, 1, 1, test1);

    // Test that after eviction one cache line is still successfully used for multiple words
    vector<uint32_t> test2 = {0b000, 0b100, 0b1000, 0b1100};
    test_cache(2, 8, 8, 1, 2, 2, test2);

    // Test that LRU eviction leads to 5 entire misses
    vector<uint32_t> test3 = {0b0000, 0b0100, 0b1000, 0b1100, 0b0000};
    test_cache(3, 12, 4, 3, 0, 5, test3);

    // Test that LRU eviction in one index's cache locations does not affect another location
    // 0 1 10 11 100 101 0 1
    vector<uint32_t> test4 = {0b000, 0b100, 0b1000, 0b1100, 0b10000, 0b10100, 0b000, 0b100};
    test_cache(4, 16, 4, 2, 0, 8, test4);

    return 0;
}
