# ====== Config ======
CXX       := g++
INCLUDE   := include
SRC_DIR   := src
BIN_DIR   := bin

CXXFLAGS  := -std=c++17 -I$(INCLUDE) -Wall -Wextra -Wpedantic -g -O2
COVFLAGS  := -O0 -g --coverage

# Sources
SRC_GRAPH := $(SRC_DIR)/graph/Graph.cpp
SRC_EULER := $(SRC_DIR)/algo/Euler.cpp

# Parts (mains)
PART1_MAIN := part1/main.cpp
PART2_MAIN := part2/main.cpp
PART3_MAIN := part3/main.cpp

# Tests
TEST_MAIN  := tests/test_euler.cpp

# ====== Phonies ======
.PHONY: all bin part1 part2 part3 run-tests coverage clean clean_coverage

# Build all demo parts
all: part1 part2 part3

# Ensure bin dir exists
bin:
	mkdir -p $(BIN_DIR)

# ----- Part 1 -----
part1: bin
	$(CXX) $(CXXFLAGS) $(SRC_GRAPH) $(PART1_MAIN) -o $(BIN_DIR)/part1

# ----- Part 2 -----
part2: bin
	$(CXX) $(CXXFLAGS) $(SRC_GRAPH) $(SRC_EULER) $(PART2_MAIN) -o $(BIN_DIR)/part2

# ----- Part 3 -----
part3: bin
	$(CXX) $(CXXFLAGS) $(SRC_GRAPH) $(SRC_EULER) $(PART3_MAIN) -o $(BIN_DIR)/part3

# ----- Tests (normal build) -----
run-tests: bin
	$(CXX) $(CXXFLAGS) $(SRC_GRAPH) $(SRC_EULER) $(TEST_MAIN) -o $(BIN_DIR)/tests
	./$(BIN_DIR)/tests

# ----- Coverage (HTML) -----
coverage: clean_coverage bin
	$(CXX) $(CXXFLAGS) $(COVFLAGS) $(SRC_GRAPH) $(SRC_EULER) $(TEST_MAIN) -o $(BIN_DIR)/tests
	./$(BIN_DIR)/tests
	# capture raw coverage
	lcov --capture --directory . --output-file coverage.info
	# filter out system headers so only YOUR files remain
	lcov --remove coverage.info '/usr/*' --output-file coverage.info
	# generate HTML
	genhtml coverage.info --output-directory coverage
	@echo "Coverage HTML -> coverage/index.html"

# ----- Clean coverage artifacts only -----
clean_coverage:
	rm -rf $(BIN_DIR) gmon.out coverage coverage.info *.gcno *.gcda **/*.gcno **/*.gcda

# ----- Full clean -----
clean:
	rm -rf $(BIN_DIR) gmon.out coverage coverage.info *.gcno *.gcda
