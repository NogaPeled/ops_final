# ====== Compiler ======
CXX  := g++
STD  := -std=c++17
WARN := -Wall -Wextra -Wpedantic
OPT  := -g -O2

# ====== Project root (robust) ======
# Absolute path of the project root (one level up from part7/)
PRJ := $(abspath $(CURDIR)/..)

# ====== Include path ======
INC := -I"$(PRJ)/include"

# ====== Binaries & sources ======
BIN_DIR    := bin
SERVER_BIN := $(BIN_DIR)/server
CLIENT_BIN := $(BIN_DIR)/client

SERVER_SRCS := \
  $(PRJ)/src/graph/Graph.cpp \
  $(PRJ)/src/algo/Euler.cpp \
  $(PRJ)/src/algo/AlgorithmFactory.cpp \
  server.cpp

CLIENT_SRCS := client.cpp

# ====== Phonies ======
.PHONY: all clean run-server run-client run-client-random run-client-manual print-%

# Build both by default
all: $(SERVER_BIN) $(CLIENT_BIN)

# Ensure bin exists
$(BIN_DIR):
	mkdir -p "$@"

# ---- Build rules ----
$(SERVER_BIN): $(BIN_DIR) $(SERVER_SRCS)
	$(CXX) $(STD) $(WARN) $(OPT) $(INC) $(SERVER_SRCS) -o "$@"

$(CLIENT_BIN): $(BIN_DIR) $(CLIENT_SRCS)
	$(CXX) $(STD) $(WARN) $(OPT) $(INC) $(CLIENT_SRCS) -o "$@"

# ---- Run helpers ----
run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

# Free-form client.
# Example:
#   make run-client CMD='ALG SCC RANDOM 8 12 7 --directed'
#   make run-client CMD='ALG HAMILTON MANUAL 4 : 0-1 1-2 2-3 3-0'
run-client: $(CLIENT_BIN)
	@if [ -z "$(CMD)" ]; then \
	  echo 'Usage: make run-client CMD="ALG <MST|SCC|MAXFLOW|HAMILTON> RANDOM <V> <E> <SEED> [--directed]"'; \
	  echo '   or: make run-client CMD="ALG <...> MANUAL <V> : u-v u-v ... [--directed]"'; \
	  exit 1; \
	fi
	./$(CLIENT_BIN) $(CMD)

# Convenience: random graph
ALGO ?= MST
V ?= 4
E ?= 4
SEED ?= 1
DIRECTED ?=
run-client-random: $(CLIENT_BIN)
	./$(CLIENT_BIN) ALG $(ALGO) RANDOM $(V) $(E) $(SEED) $(DIRECTED)

# Convenience: manual graph (0-based)
# Example:
#   make run-client-manual ALGO=HAMILTON V=4 EDGES="0-1 1-2 2-3 3-0"
run-client-manual: $(CLIENT_BIN)
	@if [ -z "$(EDGES)" ]; then \
	  echo 'Usage: make run-client-manual ALGO=<...> V=<n> EDGES="0-1 1-2 ..." [DIRECTED=--directed]'; \
	  exit 1; \
	fi
	./$(CLIENT_BIN) ALG $(ALGO) MANUAL $(V) : $(EDGES) $(DIRECTED)

# Clean
clean:
	rm -rf "$(BIN_DIR)"

# Debug helper
print-%:
	@printf '%s="'$*'"\n'; printf '%s\n' "$($*)"
