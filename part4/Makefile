# =========================
# Makefile (part4/)
# =========================

# ---- Toolchain ----
CXX := g++
STD := -std=c++17
WARN := -Wall -Wextra -Wpedantic

# ---- Repo root (absolute path) ----
ROOT := $(abspath ..)

# ---- Includes & sources (absolute; robust for lcov) ----
INCS  := -I$(ROOT)/include
SRC   := $(ROOT)/src/graph/Graph.cpp $(ROOT)/src/algo/Euler.cpp
PART3 := $(ROOT)/part3/main.cpp
TESTS := $(ROOT)/tests/test_euler.cpp

# ---- Local outputs (live under part4/) ----
BIN      := bin
TEST_BIN := $(BIN)/tests
RUN_BIN  := $(BIN)/part3
ART      := artifacts

# ---- Auto-open HTML (WSL / Linux / macOS / Windows) ----
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

# ---- Phonies ----
.PHONY: all clean test coverage memcheck callgrind gprof reports print-%

# ---- Ensure dirs ----
$(BIN) $(ART):
	mkdir -p "$@"

# =========================
# Build
# =========================
all: $(RUN_BIN)

# Part 3 runnable (used by valgrind/gprof)
$(RUN_BIN): $(BIN) $(SRC) $(PART3)
	$(CXX) $(STD) $(WARN) -g -O2 $(INCS) $(SRC) $(PART3) -o "$@"

# Tests (with coverage). -fprofile-abs-path fixes lcov path issues.
$(TEST_BIN): $(BIN) $(SRC) $(TESTS)
	$(CXX) $(STD) $(WARN) -g -O0 --coverage -fprofile-abs-path $(INCS) $(SRC) $(TESTS) -o "$@"

# =========================
# Test
# =========================
test: $(TEST_BIN)
	"$(TEST_BIN)"

# =========================
# Coverage (lcov + genhtml)
# =========================
coverage: clean $(TEST_BIN) $(ART)
	@echo ">>> Running tests for coverage"
	"$(TEST_BIN)"
	@echo ">>> Capturing coverage"
	lcov --capture \
	     --directory "$(BIN)" \
	     --base-directory "$(ROOT)" \
	     --output-file raw.info \
	     --ignore-errors mismatch,gcov \
	     --rc branch_coverage=1 \
	     --rc geninfo_unexecuted_blocks=1
	@echo ">>> Filtering out system + doctest"
	lcov --remove raw.info '/usr/*' '*/tests/doctest*' -o coverage.info
	@echo ">>> Rendering HTML"
	genhtml --rc branch_coverage=1 coverage.info --output-directory coverage_html
	@echo ">>> Saving artifacts"
	cp raw.info coverage.info "$(ART)/"
	@echo ">>> Opening report"
	$(OPEN_HTML) coverage_html/index.html || true

# =========================
# Valgrind (memcheck)
# =========================
memcheck: $(RUN_BIN) $(ART)
	@echo ">>> Valgrind (undirected)"
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	  "$(RUN_BIN)" -v 8 -e 12 -s 1 2> "$(ART)/memcheck_undirected.txt" || true
	@echo ">>> Valgrind (directed)"
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	  "$(RUN_BIN)" -v 8 -e 20 -s 1 --directed 2> "$(ART)/memcheck_directed.txt" || true
	@echo "See $(ART)/memcheck_*.txt"

# =========================
# Callgrind
# =========================
callgrind: $(RUN_BIN) $(ART)
	@echo ">>> Running callgrind"
	valgrind --tool=callgrind "$(RUN_BIN)" -v 4000 -e 6000 -s 1 >/dev/null
	@f=$$(ls -t callgrind.out.* | head -n1); \
	echo ">>> Latest callgrind file: $$f"; \
	callgrind_annotate "$$f" > "$(ART)/callgrind_annotate.txt"; \
	cp "$$f" "$(ART)/"; \
	echo "See $(ART)/callgrind_annotate.txt and $(ART)/$$f"

# =========================
# gprof
# =========================
gprof: $(ART)
	$(CXX) $(STD) $(WARN) -g -O2 -pg $(INCS) $(SRC) $(PART3) -o "$(RUN_BIN)"
	@echo ">>> Running profiled binary"
	"$(RUN_BIN)" -v 4000 -e 6000 -s 1 >/dev/null
	gprof "$(RUN_BIN)" gmon.out > "$(ART)/gprof.txt"
	@echo "See $(ART)/gprof.txt"

# =========================
# Convenience
# =========================
reports: coverage memcheck callgrind gprof
	@echo "All artifacts in ./$(ART) and ./coverage_html"

# =========================
# Clean
# =========================
clean:
	rm -rf "$(BIN)" coverage.info raw.info coverage_html gmon.out callgrind.out.* "$(ART)"

# Debug: print any var (e.g., `make print-ROOT`)
print-%:
	@printf '%s="'$*'"\n'; printf '%s$$\n' "$($*)"
