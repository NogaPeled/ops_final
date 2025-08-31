# ====== Compiler & flags ======
CXX  := g++
STD  := -std=c++17
WARN := -Wall -Wextra -Wpedantic
DBG  := -g
OPT  := -O2

# ====== Project root (absolute) ======
PRJ := $(abspath $(CURDIR)/..)

# ====== Includes ======
INC := -I"$(PRJ)/include"

# ====== Binary & dirs ======
BIN_DIR := bin
APP     := $(BIN_DIR)/part3_random

# ====== Sources ======
# Core project sources (Graph + Euler)
CORE_SRCS := \
  $(PRJ)/src/graph/Graph.cpp \
  $(PRJ)/src/algo/Euler.cpp

# Your Part 3 program lives here:
APP_SRC := main.cpp

# ====== Phony targets ======
.PHONY: all run run-directed clean

# ====== Build ======
all: $(APP)

$(BIN_DIR):
	mkdir -p "$@"

$(APP): $(BIN_DIR) $(CORE_SRCS) $(APP_SRC)
	$(CXX) $(STD) $(WARN) $(DBG) $(OPT) $(INC) \
	  $(CORE_SRCS) $(APP_SRC) -o "$@"

# ====== Run helpers ======
# Defaults (override on make line: V=8 E=12 SEED=7)
V ?= 6
E ?= 7
SEED ?= 1

run: $(APP)
	./$(APP) -v $(V) -e $(E) -s $(SEED)

run-directed: $(APP)
	./$(APP) -v $(V) -e $(E) -s $(SEED) --directed

# ====== Clean ======
clean:
	rm -rf "$(BIN_DIR)"