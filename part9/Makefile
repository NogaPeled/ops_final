# ====== Compiler ======
CXX  := g++
STD  := -std=c++17
WARN := -Wall -Wextra -Wpedantic
OPT  := -g -O2

# ====== Project root (absolute, robust) ======
PRJ := $(abspath $(CURDIR)/..)

# ====== Include path ======
INC := -I"$(PRJ)/include"

# ====== Binaries & sources ======
BIN_DIR     := bin
SERVER_BIN  := $(BIN_DIR)/server_pipeline
CLIENT_BIN  := $(BIN_DIR)/client

SERVER_SRCS := \
  $(PRJ)/src/graph/Graph.cpp \
  $(PRJ)/src/algo/Euler.cpp \
  $(PRJ)/src/algo/AlgorithmFactory.cpp \
  server_pipeline.cpp

# Use the Part-7 client (no need to duplicate); change path if you copied it.
CLIENT_SRCS := ../part7/client.cpp

.PHONY: all clean run-server run-client run-client-random run-client-manual

all: $(SERVER_BIN) $(CLIENT_BIN)

$(BIN_DIR):
	mkdir -p "$@"

$(SERVER_BIN): $(BIN_DIR) $(SERVER_SRCS)
	$(CXX) $(STD) $(WARN) $(OPT) $(INC) $(SERVER_SRCS) -o "$@"

$(CLIENT_BIN): $(BIN_DIR) $(CLIENT_SRCS)
	$(CXX) $(STD) $(WARN) $(OPT) $(INC) $(CLIENT_SRCS) -o "$@"

# -------- Run server --------
run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

# -------- Free-form client (same syntax as part 8/7) --------
# Examples:
#   make run-client CMD='ALG ALL RANDOM 8 12 7 --directed'
#   make run-client CMD='ALG ALL MANUAL 4 : 0-1 1-2 2-3 3-0'
run-client: $(CLIENT_BIN)
	@if [ -z "$(CMD)" ]; then \
	  echo 'Usage: make run-client CMD="ALG ALL RANDOM <V> <E> <SEED> [--directed]"'; \
	  echo '   or: make run-client CMD="ALG ALL MANUAL <V> : u-v u-v ... [--directed]"'; \
	  exit 1; \
	fi
	./$(CLIENT_BIN) $(CMD)

# Convenience: RANDOM
V ?= 8
E ?= 12
SEED ?= 7
DIRECTED ?=
run-client-random: $(CLIENT_BIN)
	./$(CLIENT_BIN) ALG ALL RANDOM $(V) $(E) $(SEED) $(DIRECTED)

# Convenience: MANUAL
# Example:
#   make run-client-manual V=4 EDGES="0-1 1-2 2-3 3-0"
run-client-manual: $(CLIENT_BIN)
	@if [ -z "$(EDGES)" ]; then \
	  echo 'Usage: make run-client-manual V=<n> EDGES="0-1 1-2 ..." [DIRECTED=--directed]'; \
	  exit 1; \
	fi
	./$(CLIENT_BIN) ALG ALL MANUAL $(V) : $(EDGES) $(DIRECTED)

clean:
	rm -rf "$(BIN_DIR)"
