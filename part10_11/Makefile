# ===================== Makefile (part10_11) =====================

# ---- Compiler & flags ----
CXX  := g++
STD  := -std=c++17
WARN := -Wall -Wextra -Wpedantic
DBG  := -g
OPT  := -O0

# ---- Project root (absolute) ----
PRJ := $(abspath $(CURDIR)/..)

# ---- Includes ----
INC := -I"$(PRJ)/include"

# ---- Common sources under src/ ----
SRCS := \
  $(PRJ)/src/graph/Graph.cpp \
  $(PRJ)/src/algo/Euler.cpp \
  $(PRJ)/src/algo/AlgorithmFactory.cpp

# ---- Where artifacts go ----
BIN_DIR := bin
ART_DIR := artifacts

# ---- Detect tests (two suites) ----
# Part-4 legacy tests
TESTS_PART4 := $(wildcard $(PRJ)/tests/test_euler.cpp)

# New tests for parts 7–11: prefer repo-root; fallback to local (part10_11/tests)
TESTS_NEW := $(firstword \
  $(wildcard $(PRJ)/tests/test_algorithms.cpp) \
  $(wildcard tests/test_algorithms.cpp))

# Helpful notices (not fatal)
ifeq ($(strip $(TESTS_PART4)),)
  $(warning Could not find $(PRJ)/tests/test_euler.cpp; Part-4 tests will be skipped)
endif
ifeq ($(strip $(TESTS_NEW)),)
  $(warning Could not find tests/test_algorithms.cpp; new tests will be skipped)
endif

# ---- Test binaries ----
TEST_BIN_1 := $(BIN_DIR)/tests_part4
TEST_BIN_2 := $(BIN_DIR)/tests_new

# ---- HTML opener (WSL/Ubuntu/macOS/Win) ----
ifeq ($(OS),Windows_NT)
  OPEN_HTML = cmd.exe /C start
else
  ifeq ($(shell command -v wslview >/dev/null 2>&1 && echo yes),yes)
    OPEN_HTML = wslview
  else ifeq ($(shell command -v xdg-open >/dev/null 2>&1 && echo yes),yes)
    OPEN_HTML = xdg-open
  else ifeq ($(shell uname -s),Darwin)
    OPEN_HTML = open
  else
    OPEN_HTML = :
  endif
endif

.PHONY: all clean test coverage memcheck callgrind gprof reports print-%

# ---- Dirs ----
$(BIN_DIR) $(ART_DIR):
	mkdir -p "$@"

# ---- Build tests with coverage ----
# (Only built if their source lists are non-empty)
ifneq ($(strip $(TESTS_PART4)),)
$(TEST_BIN_1): $(BIN_DIR) $(SRCS) $(TESTS_PART4)
	$(CXX) $(STD) $(WARN) $(DBG) $(OPT) --coverage -fprofile-abs-path $(INC) \
	  $(SRCS) $(TESTS_PART4) -o "$@"
endif

ifneq ($(strip $(TESTS_NEW)),)
$(TEST_BIN_2): $(BIN_DIR) $(SRCS) $(TESTS_NEW)
	$(CXX) $(STD) $(WARN) $(DBG) $(OPT) --coverage -fprofile-abs-path $(INC) \
	  $(SRCS) $(TESTS_NEW) -o "$@"
endif

# ---- Default: build whatever exists ----
all: $(BIN_DIR)
ifneq ($(strip $(TESTS_PART4)),)
all: $(TEST_BIN_1)
endif
ifneq ($(strip $(TESTS_NEW)),)
all: $(TEST_BIN_2)
endif

# ---- Run tests ----
test: all
ifneq ($(strip $(TESTS_PART4)),)
	"$(TEST_BIN_1)"
endif
ifneq ($(strip $(TESTS_NEW)),)
	"$(TEST_BIN_2)"
endif

# ---- Coverage (lcov + genhtml) ----
coverage: clean all $(ART_DIR)
	@echo ">>> Running tests"
ifneq ($(strip $(TESTS_PART4)),)
	"$(TEST_BIN_1)"
endif
ifneq ($(strip $(TESTS_NEW)),)
	"$(TEST_BIN_2)"
endif
	@echo ">>> Capturing coverage"
	lcov --capture \
	     --directory "$(BIN_DIR)" \
	     --base-directory "$(PRJ)" \
	     --output-file raw.info \
	     --ignore-errors mismatch,gcov \
	     --rc branch_coverage=1 \
	     --rc geninfo_unexecuted_blocks=1
	@echo ">>> Filtering out system + doctest"
	lcov --remove raw.info '/usr/*' '*/tests/doctest*' -o coverage.info
	@echo ">>> Deriving function coverage from line hits (fixes header/inline undercount)"
	lcov --derive-func-data coverage.info -o coverage_derived.info
	@echo ">>> Rendering HTML"
	genhtml --rc branch_coverage=1 coverage_derived.info --output-directory coverage_html
	@echo ">>> Saving artifacts"
	cp raw.info coverage.info coverage_derived.info "$(ART_DIR)/"
	@echo ">>> Opening report"
	$(OPEN_HTML) coverage_html/index.html || true

# ---- Valgrind (memcheck) ----
memcheck: all $(ART_DIR)
	@echo ">>> Valgrind memcheck"
ifneq ($(strip $(TESTS_PART4)),)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	  "$(TEST_BIN_1)" 2> "$(ART_DIR)/memcheck_part4.txt" || true
endif
ifneq ($(strip $(TESTS_NEW)),)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	  "$(TEST_BIN_2)" 2> "$(ART_DIR)/memcheck_new.txt" || true
endif
	@echo "See $(ART_DIR)/memcheck_*.txt"

# ---- Callgrind (named outputs per suite) ----
callgrind: all $(ART_DIR)
ifneq ($(strip $(TESTS_PART4)),)
	valgrind --tool=callgrind --callgrind-out-file="$(ART_DIR)/callgrind_part4.out" "$(TEST_BIN_1)" >/dev/null
	callgrind_annotate "$(ART_DIR)/callgrind_part4.out" > "$(ART_DIR)/callgrind_annotate_part4.txt"
endif
ifneq ($(strip $(TESTS_NEW)),)
	valgrind --tool=callgrind --callgrind-out-file="$(ART_DIR)/callgrind_new.out" "$(TEST_BIN_2)" >/dev/null
	callgrind_annotate "$(ART_DIR)/callgrind_new.out" > "$(ART_DIR)/callgrind_annotate_new.txt"
endif
	@echo "See $(ART_DIR)/callgrind_*.out and *_annotate_*.txt"

# ---- gprof (separate outputs using GMON_OUT_PREFIX) ----
gprof: all $(ART_DIR)
ifneq ($(strip $(TESTS_PART4)),)
	$(CXX) $(STD) $(WARN) -pg $(DBG) $(OPT) $(INC) $(SRCS) $(TESTS_PART4) -o "$(TEST_BIN_1)"
	@echo ">>> Running profiled tests (part4)"
	GMON_OUT_PREFIX="$(ART_DIR)/gmon_part4" "$(TEST_BIN_1)" >/dev/null
	@f=$$(ls -t $(ART_DIR)/gmon_part4.* | head -n1); \
	gprof "$(TEST_BIN_1)" "$$f" > "$(ART_DIR)/gprof_part4.txt"
endif
ifneq ($(strip $(TESTS_NEW)),)
	$(CXX) $(STD) $(WARN) -pg $(DBG) $(OPT) $(INC) $(SRCS) $(TESTS_NEW) -o "$(TEST_BIN_2)"
	@echo ">>> Running profiled tests (new)"
	GMON_OUT_PREFIX="$(ART_DIR)/gmon_new" "$(TEST_BIN_2)" >/dev/null
	@f=$$(ls -t $(ART_DIR)/gmon_new.* | head -n1); \
	gprof "$(TEST_BIN_2)" "$$f" > "$(ART_DIR)/gprof_new.txt"
endif
	@echo "See $(ART_DIR)/gprof_*.txt"

# ---- One button to get everything ----
reports: coverage memcheck callgrind gprof
	@echo "All artifacts in ./$(ART_DIR) and ./coverage_html"

# ---- Clean ----
clean:
	rm -rf "$(BIN_DIR)" "$(ART_DIR)" coverage.info raw.info coverage_derived.info coverage_html gmon.out callgrind.out.*
	@find "$(PRJ)" -name '*.gcda' -o -name '*.gcno' -delete 2>/dev/null || true

# ---- Debug helper ----
print-%:
	@printf '%s="'$*'"\n'; printf '%s\n' "$($*)"

# ===============================================================
