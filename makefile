# Makefile

## Examples:
# make sim_cycle # build sim_cycle
# make sim_funct # build sim_funct
# make all # build sim_funct, sim_cycle and all tests
# make debug # build debug version of sim_funct, sim_cycle and all tests
# make tests # build all tests

# Compiler settings
CC = g++
CFLAGS = --std=c++14 -Wall -O3
DFLAGS = -g -pedantic

# Source and header files
SIM_FUNCT_SRCS = sim_funct.cpp funct.cpp emulator.cpp MemoryStore.cpp Utilities.cpp
SIM_CYCLE_SRCS = sim_cycle.cpp cycle.cpp cache.cpp emulator.cpp MemoryStore.cpp Utilities.cpp
COMMON_HDRS = $(wildcard *.h)

# Main targets
all: sim_funct sim_cycle tests

sim_funct: $(SIM_FUNCT_SRCS) $(COMMON_HDRS)
	$(CC) $(CFLAGS) -o sim_funct $(SIM_FUNCT_SRCS)

sim_cycle: $(SIM_CYCLE_SRCS) $(COMMON_HDRS)
	$(CC) $(CFLAGS) -o sim_cycle $(SIM_CYCLE_SRCS)

# Compile test_cycle_*.cpp
test_cycle_%: test_cycle_%.cpp cycle.cpp cache.cpp emulator.cpp MemoryStore.cpp Utilities.cpp $(COMMON_HDRS)
	$(CC) $(CFLAGS) -o $@ $< cycle.cpp cache.cpp emulator.cpp MemoryStore.cpp Utilities.cpp

# Compile test_funct_*.cpp
test_funct_%: test_funct_%.cpp funct.cpp emulator.cpp MemoryStore.cpp Utilities.cpp $(COMMON_HDRS)
	$(CC) $(CFLAGS) -o $@ $< funct.cpp emulator.cpp MemoryStore.cpp Utilities.cpp

# Compile other test_*.cpp
test_%: test_%.cpp emulator.cpp MemoryStore.cpp Utilities.cpp $(COMMON_HDRS)
	$(CC) $(CFLAGS) -o $@ $< emulator.cpp MemoryStore.cpp Utilities.cpp

# Test targets
tests: $(patsubst %.cpp,%,$(wildcard test_cycle_*.cpp)) $(patsubst %.cpp,%,$(wildcard test_funct_*.cpp)) $(patsubst %.cpp,%,$(wildcard test_*.cpp))

# Debug builds
debug: DFLAGS += -DDEBUG
debug: CFLAGS += $(DFLAGS)
debug: all

# Clean function
clean:
	rm -f sim_funct sim_cycle
	find . -type f -name 'test_*' ! -name '*.cpp' -exec rm {} +

# Phony targets
.PHONY: all debug tests clean
