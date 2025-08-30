# Parts 10–11 — Valgrind Analysis + Code Coverage

This part adds **tooling and proof** around your code quality:

* **Part 10**: Run Valgrind tools

  * `memcheck` – leaks & invalid memory
  * `helgrind` – data races (multithreading)
  * `callgrind` – CPU profiling (call graph)

* **Part 11**: Generate **HTML code coverage** with LCOV/genhtml.

Everything is wired in `part10_11/Makefile`.

---

## Layout

```
os_project/
  include/
  src/
  tests/
    test_euler.cpp           # existing Part 4 tests
    test_algorithms.cpp      # new tests for parts 7–11
  part10_11/
    Makefile                 # valgrind + coverage tooling (this part)
    (artifacts, coverage_html created by the makefile)
```

The Makefile builds **two test binaries**:

* `tests_part4` → runs `tests/test_euler.cpp`
* `tests_new`   → runs `tests/test_algorithms.cpp`

Both binaries are included in coverage + Valgrind runs.

---

## Prerequisites

Ubuntu / Debian / WSL:

```bash
sudo apt update
sudo apt install -y build-essential valgrind lcov
```

(Optionally install `wslu` for `wslview` to auto-open the HTML report on WSL.)

---

## Quick start

From `os_project/part10_11`:

```bash
make reports
```

This will:

1. Build tests (with coverage flags)
2. Run tests (both binaries)
3. Capture LCOV → `coverage_html/index.html`
4. Run Valgrind:

   * memcheck → `artifacts/memcheck_part4.txt`, `artifacts/memcheck_new.txt`
   * helgrind → `artifacts/helgrind_part4.txt`, `artifacts/helgrind_new.txt`
   * callgrind → `callgrind.out.*` + annotated dumps in `artifacts/`
5. Run gprof → `artifacts/gprof_part4.txt`, `artifacts/gprof_new.txt`

Open the HTML report:

* Auto-opens if your system supports `wslview`, `xdg-open`, or `open`.
* Otherwise: open `coverage_html/index.html` manually.

---

## Common commands

```bash
# Build both test binaries
make

# Run tests (both)
make test

# Coverage: runs tests, captures LCOV, renders HTML
make coverage

# Valgrind (memcheck): leaks/invalid memory
make memcheck
# Output → artifacts/memcheck_part4.txt, artifacts/memcheck_new.txt

# Helgrind: data races (for multithreaded code)
make helgrind
# Output → artifacts/helgrind_part4.txt, artifacts/helgrind_new.txt

# Callgrind: CPU profiling / call graph
make callgrind
# Output → callgrind.out.* + artifacts/callgrind_annotate_*.txt

# gprof: classic profiler
make gprof
# Output → artifacts/gprof_part4.txt, artifacts/gprof_new.txt

# Everything (coverage + memcheck + helgrind + callgrind + gprof)
make reports

# Clean all generated files
make clean
```

**Artifacts** land in `./artifacts`. Coverage HTML lands in `./coverage_html`.

---

## What “counts” toward the requirements

* **10. Valgrind analysis**
  `make reports` produces:

  * `artifacts/memcheck_*.txt` (Memcheck)
  * `artifacts/helgrind_*.txt` (Helgrind)
  * `artifacts/callgrind_annotate_*.txt` + `callgrind.out.*` (Callgrind)
  * `artifacts/gprof_*.txt` (extra profiler evidence)

* **11. Code coverage**
  `make coverage` renders an **HTML** report in `coverage_html/` showing line & function coverage across:

  * `src/` implementation files
  * your headers (inline code) if executed
  * both test binaries

Include screenshots or the generated HTML folder in your submission if needed.

---

## Examples (typical console summary)

```
Overall coverage rate:
  lines......: 99.3%
  functions..: 96.9%
```
---

