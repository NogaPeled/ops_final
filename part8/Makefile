# ====== Compiler ======
CXX  := g++
STD  := -std=c++17
WARN := -Wall -Wextra -Wpedantic
OPT  := -g -O2

# ====== Project root (absolute, robust) ======
PRJ := $(abspath $(CURDIR)/..)

# ====== Include path ======
INC := -I"$(PRJ)/include"

# ====== Binaries ======
BIN_DIR    := bin
SERVER_LF  := $(BIN_DIR)/server_lf
CLIENT_BIN := $(BIN_DIR)/client

# ====== Pick server main automatically: prefer server_lf.cpp then server.cpp ======
SERVER_MAIN := $(firstword $(wildcard server_lf.cpp server.cpp))
ifeq ($(SERVER_MAIN),)
  $(error No server source found. Put server_lf.cpp (LF server) or server.cpp in part8/)
endif

# ====== Pick client automatically: prefer local client.cpp, else part7/client.cpp ======
# You can also override at the command line: make run-client CLIENT_MAIN=/path/to/client.cpp
CLIENT_MAIN ?= $(firstword $(wildcard client.cpp ../part7/client.cpp $(PRJ)/part7/client.cpp))
ifeq ($(CLIENT_MAIN),)
  $(error No client source found. Put client.cpp in part8/ or ensure part7/client.cpp exists, \
or run make with CLIENT_MAIN=/absolute/path/to/client.cpp)
endif

# ====== Server sources (link against your shared code in /src) ======
SERVER_SRCS := \
  $(PRJ)/src/graph/Graph.cpp \
  $(PRJ)/src/algo/Euler.cpp \
  $(PRJ)/src/algo/AlgorithmFactory.cpp \
  $(SERVER_MAIN)

.PHONY: all clean run-server run-client run-client-random run-client-manual

all: $(SERVER_LF) $(CLIENT_BIN)

$(BIN_DIR):
	mkdir -p "$@"

$(SERVER_LF): $(BIN_DIR) $(SERVER_SRCS)
	$(CXX) $(STD) $(WARN) $(OPT) $(INC) $(SERVER_SRCS) -o "$@"

$(CLIENT_BIN): $(BIN_DIR) $(CLIENT_MAIN)
	$(CXX) $(STD) $(WARN) $(OPT) $(INC) "$(CLIENT_MAIN)" -o "$@"

# -------- Run LF server --------
run-server: $(SERVER_LF)
	./$(SERVER_LF)

# -------- Client helpers (same syntax as part 8) --------
# Free-form:
#   make run-client CMD='ALG ALL RANDOM 8 12 7 --directed'
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
