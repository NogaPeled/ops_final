# ==== compiler flags ====
CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -Wpedantic -g -O2

# ==== where the project root is (one level up from part6) ====
PROJECT_ROOT := $(abspath ..)

# include + source dirs at the project root
INCLUDE_DIR  := $(PROJECT_ROOT)/include
SRC_GRAPH    := $(PROJECT_ROOT)/src/graph/Graph.cpp
SRC_EULER    := $(PROJECT_ROOT)/src/algo/Euler.cpp

# local outputs
BIN := bin

# build both by default
all: $(BIN)/server $(BIN)/client

# ensure bin exists
$(BIN):
	mkdir -p "$@"

# ---- server ----
$(BIN)/server: $(BIN) server.cpp $(SRC_GRAPH) $(SRC_EULER)
	$(CXX) $(CXXFLAGS) -I"$(INCLUDE_DIR)" \
	    $(SRC_GRAPH) $(SRC_EULER) server.cpp -o "$@"

# ---- client ----
$(BIN)/client: $(BIN) client.cpp
	$(CXX) $(CXXFLAGS) -I"$(INCLUDE_DIR)" client.cpp -o "$@"

# convenience
run-server: $(BIN)/server
	./$(BIN)/server

run-client-random: $(BIN)/client
	./$(BIN)/client RANDOM 8 12 1

clean:
	rm -rf "$(BIN)"

.PHONY: all run-server run-client-random clean
