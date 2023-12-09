#!/bin/bash

if [ ! -f "sim_cycle" ]
then
  touch sim_cycle
fi

g++ -o sim_cycle sim_cycle.cpp cycle.cpp cache.cpp emulator.cpp MemoryStore.cpp Utilities.cpp
chmod +rwx sim_cycle
echo "note: sim_cycle <file.bin> <cache_config.txt>"