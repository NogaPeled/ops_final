# ====== Compiler ======
CXX  := g++
STD  := -std=c++17
WARN := -Wall -Wextra -Wpedantic
DBG  := -g
OPT  := -O2

# ====== Project root (absolute) ======
PRJ := $(abspath $(CURDIR)/..)

# ====== Includes ======
INC := -I"$(PRJ)/include"

# ====== Sources ======
# Change APP_SRC if your Part-3 file has a different name.
APP_SRC := part3_random.cpp

CORE_SRCS := \
  $(PRJ)/src/graph/Graph.cpp \
  $(PRJ)/src/algo/Euler.cpp

# ====== Binaries & dirs ======
BIN_DIR := bin
APP_BIN := $(BIN_DIR)/part3_random

.PHONY: all clean run run-directed

all: $(APP_BIN)

$(BIN_DIR):
	mkdir -p "$@"

$(APP_BIN): $(BIN_DIR) $(APP_SRC) $(CORE_SRCS)
	$(CXX) $(STD) $(WARN) $(DBG) $(OPT) $(INC) $(APP_SRC) $(CORE_SRCS) -o "$@"

# ---- Convenience runners (default values you can override) ----
V ?= 6
E ?= 7
SEED ?= 1

run: $(APP_BIN)
	./$(APP_BIN) -v $(V) -e $(E) -s $(SEED)

run-directed: $(APP_BIN)
	./$(APP_BIN) -v $(V) -e $(E) -s $(SEED) --directed

clean:
	rm -rf "$(BIN_DIR)"
