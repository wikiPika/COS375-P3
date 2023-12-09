#!/bin/bash

if [ ! -f "sim_funct" ]
then
  touch sim_funct
fi

g++ -o sim_funct sim_funct.cpp funct.cpp emulator.cpp MemoryStore.cpp Utilities.cpp
chmod +rwx sim_funct
echo "note: sim_funct <file.bin>"