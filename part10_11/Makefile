# ====== Compiler & flags ======
CXX  := g++
STD  := -std=c++17
WARN := -Wall -Wextra -Wpedantic
DBG  := -g
OPT  := -O0

# ====== Project root (absolute) ======
PRJ := $(abspath $(CURDIR)/..)

# ====== Includes ======
INC := -I"$(PRJ)/include"

# ====== Binaries & dirs ======
BIN_DIR    := bin
ART_DIR    := artifacts
TEST_BIN_1 := $(BIN_DIR)/tests_part4
TEST_BIN_2 := $(BIN_DIR)/tests_new

# ====== Sources (library/impl) ======
SRCS := \
  $(PRJ)/src/graph/Graph.cpp \
  $(PRJ)/src/algo/Euler.cpp \
  $(PRJ)/src/algo/AlgorithmFactory.cpp

# ====== Tests split into two binaries ======
# Part-4 legacy tests (keep their own main)
TESTS_PART4 := $(PRJ)/tests/test_euler.cpp

# New tests for parts 7–11
# Prefer repo-root tests; fall back to local part10_11/tests
TESTS_NEW := $(wildcard $(PRJ)/tests/test_algorithms.cpp)
ifeq ($(strip $(TESTS_NEW)),)
  TESTS_NEW := $(wildcard tests/test_algorithms.cpp)
endif

ifeq ($(strip $(TESTS_PART4)),)
  $(warning Could not find $(PRJ)/tests/test_euler.cpp; Part-4 tests will be skipped)
endif
ifeq ($(strip $(TESTS_NEW)),)
  $(warning Could not find tests/test_algorithms.cpp; new tests will be skipped)
endif

# ====== HTML opener detection ======
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

# ====== Phonies ======
.PHONY: all clean test coverage memcheck helgrind callgrind gprof reports print-%

# ====== Directories ======
$(BIN_DIR) $(ART_DIR):
	mkdir -p "$@"

# ====== Build test binaries with coverage ======
$(TEST_BIN_1): $(BIN_DIR) $(SRCS) $(TESTS_PART4)
	$(CXX) $(STD) $(WARN) $(DBG) $(OPT) --coverage -fprofile-abs-path $(INC) \
	  $(SRCS) $(TESTS_PART4) -o "$@"

$(TEST_BIN_2): $(BIN_DIR) $(SRCS) $(TESTS_NEW)
	$(CXX) $(STD) $(WARN) $(DBG) $(OPT) --coverage -fprofile-abs-path $(INC) \
	  $(SRCS) $(TESTS_NEW) -o "$@"

# ====== Default: build both (only those that exist) ======
all: $(BIN_DIR)
	@true
ifneq ($(strip $(TESTS_PART4)),)
all: $(TEST_BIN_1)
endif
ifneq ($(strip $(TESTS_NEW)),)
all: $(TEST_BIN_2)
endif

# ====== Run tests ======
test: all
ifneq ($(strip $(TESTS_PART4)),)
	"$(TEST_BIN_1)"
endif
ifneq ($(strip $(TESTS_NEW)),)
	"$(TEST_BIN_2)"
endif

# ====== Coverage (lcov/genhtml) with graceful fallback ======
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

	@echo ">>> Trying to derive function coverage (optional)"
	@SRC=coverage.info; \
	if lcov --derive-func-data coverage.info -o coverage_derived.info >/dev/null 2>&1; then \
	  echo "    Derived OK -> coverage_derived.info"; \
	  SRC=coverage_derived.info; \
	else \
	  echo "    Not supported by your lcov → using coverage.info as-is"; \
	fi; \
	echo ">>> Rendering HTML"; \
	genhtml --rc branch_coverage=1 $$SRC --output-directory coverage_html; \
	echo ">>> Saving artifacts"; \
	if [ -f coverage_derived.info ]; then cp raw.info coverage.info coverage_derived.info "$(ART_DIR)/"; else cp raw.info coverage.info "$(ART_DIR)/"; fi; \
	echo ">>> Opening report"; \
	$(OPEN_HTML) coverage_html/index.html || true

# ====== Valgrind (memcheck) on tests ======
memcheck: all $(ART_DIR)
	@echo ">>> Valgrind memcheck on tests"
ifneq ($(strip $(TESTS_PART4)),)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	  "$(TEST_BIN_1)" 2> "$(ART_DIR)/memcheck_part4.txt" || true
endif
ifneq ($(strip $(TESTS_NEW)),)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	  "$(TEST_BIN_2)" 2> "$(ART_DIR)/memcheck_new.txt" || true
endif
	@echo "See $(ART_DIR)/memcheck_*.txt"

# ====== Helgrind (data race detector) on tests ======
helgrind: all $(ART_DIR)
	@echo ">>> Helgrind on tests"
ifneq ($(strip $(TESTS_PART4)),)
	-valgrind --tool=helgrind "$(TEST_BIN_1)" 2> "$(ART_DIR)/helgrind_part4.txt"
endif
ifneq ($(strip $(TESTS_NEW)),)
	-valgrind --tool=helgrind "$(TEST_BIN_2)" 2> "$(ART_DIR)/helgrind_new.txt"
endif
	@echo "See $(ART_DIR)/helgrind_*.txt"

# ====== Callgrind on tests (two runs, keep both outputs) ======
callgrind: all $(ART_DIR)
ifneq ($(strip $(TESTS_PART4)),)
	valgrind --tool=callgrind "$(TEST_BIN_1)" >/dev/null
	@f=$$(ls -t callgrind.out.* | head -n1); \
	echo ">>> Latest callgrind file (part4): $$f"; \
	callgrind_annotate "$$f" > "$(ART_DIR)/callgrind_annotate_part4.txt"; \
	cp "$$f" "$(ART_DIR)/"; \
	true
endif
ifneq ($(strip $(TESTS_NEW)),)
	valgrind --tool=callgrind "$(TEST_BIN_2)" >/dev/null
	@f=$$(ls -t callgrind.out.* | head -n1); \
	echo ">>> Latest callgrind file (new): $$f"; \
	callgrind_annotate "$$f" > "$(ART_DIR)/callgrind_annotate_new.txt"; \
	cp "$$f" "$(ART_DIR)/"; \
	true
endif

# ====== gprof on tests (separate profiles) ======
gprof: $(BIN_DIR) $(ART_DIR)
ifneq ($(strip $(TESTS_PART4)),)
	$(CXX) $(STD) $(WARN) -pg $(DBG) $(OPT) $(INC) $(SRCS) $(TESTS_PART4) -o "$(TEST_BIN_1)"
	@echo ">>> Running profiled tests (part4)"
	"$(TEST_BIN_1)" >/dev/null
	@mv -f gmon.out "$(ART_DIR)/gmon_part4.out" 2>/dev/null || true
	gprof "$(TEST_BIN_1)" "$(ART_DIR)/gmon_part4.out" > "$(ART_DIR)/gprof_part4.txt" || true
endif
ifneq ($(strip $(TESTS_NEW)),)
	$(CXX) $(STD) $(WARN) -pg $(DBG) $(OPT) $(INC) $(SRCS) $(TESTS_NEW) -o "$(TEST_BIN_2)"
	@echo ">>> Running profiled tests (new)"
	"$(TEST_BIN_2)" >/dev/null
	@mv -f gmon.out "$(ART_DIR)/gmon_new.out" 2>/dev/null || true
	gprof "$(TEST_BIN_2)" "$(ART_DIR)/gmon_new.out" > "$(ART_DIR)/gprof_new.txt" || true
endif
	@echo "See $(ART_DIR)/gprof_*.txt"

# ====== Convenience ======
reports: coverage memcheck helgrind callgrind gprof
	@echo "All artifacts in ./$(ART_DIR) and ./coverage_html"

# ====== Clean ======
clean:
	rm -rf "$(BIN_DIR)" "$(ART_DIR)" coverage.info raw.info coverage_derived.info coverage_html gmon.out callgrind.out.*
	@find "$(PRJ)" \( -name '*.gcda' -o -name '*.gcno' \) -delete 2>/dev/null || true

# ====== Debug helper ======
print-%:
	@printf '%s="'$*'"\n'; printf '%s\n' "$($*)"
