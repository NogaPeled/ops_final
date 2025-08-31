
````markdown
# Part 4 — Coverage, Memcheck, and Profiling

This folder automates **test coverage (lcov)**, **memory checking (Valgrind/memcheck)**, and **CPU profiling** with both **Callgrind** and **gprof** for the Euler + Graph project.

## What’s here

- `Makefile` — runs everything from this subfolder but builds/links sources from the project root.
- Artifacts are written to `part4/artifacts/`
- Coverage HTML goes to `part4/coverage_html/`

---

## Prerequisites

Install these once (Ubuntu/WSL):

```bash
sudo apt update
sudo apt install build-essential valgrind lcov binutils gprof
# Optional (auto-open coverage in Windows browser under WSL):
sudo apt install wslu    # provides wslview
# Optional GUI viewer for callgrind:
sudo apt install kcachegrind
````

---

## One command to generate **everything**

```bash
cd part4
make reports
```

That will:

1. Build & run unit tests with coverage
2. Generate lcov HTML report
3. Run Valgrind memcheck (two cases)
4. Run Valgrind callgrind (large case)
5. Run gprof (sampling profile)

### Where to look

* **Coverage HTML:** `part4/coverage_html/index.html`
  The Makefile tries to open it automatically with `wslview` (WSL) or `xdg-open` (Linux).
* **Memcheck reports:**

  * `part4/artifacts/memcheck_undirected.txt`
  * `part4/artifacts/memcheck_directed.txt`
    Quick leak summary:

  ```bash
  grep -E "ERROR SUMMARY|definitely lost|indirectly lost" part4/artifacts/memcheck_*.txt
  ```
* **Callgrind outputs:**

  * Raw: `part4/artifacts/callgrind.out.<PID>`
  * Annotated: `part4/artifacts/callgrind_annotate.txt`
    GUI view (optional):

  ```bash
  kcachegrind part4/artifacts/callgrind.out.<PID>
  ```
* **gprof output:**

  * `part4/artifacts/gprof.txt`

Expected coverage (from our run): **\~99% lines**, **100% functions**.
You may see harmless lcov warnings about libstdc++; we filter system headers anyway.

---

## Make targets (quick reference)

```bash
make coverage   # tests + lcov + HTML report (opens in browser if possible)
make memcheck   # valgrind leak check (directed & undirected runs)
make callgrind  # valgrind callgrind + annotated text report
make gprof      # build -pg, run once, dump gprof.txt
make clean      # remove bin/, artifacts/, coverage_html/, temporary files
make reports    # runs: coverage, memcheck, callgrind, gprof
```

---

## Changing input sizes / seeds

By default:

* `memcheck` uses `-v 8 -e 12 -s 1` (undirected) and `-v 8 -e 20 -s 1 --directed`
* `callgrind` and `gprof` use `-v 4000 -e 6000 -s 1` for a more meaningful profile

---

## Paths note (important if you move the project)

This **part4/Makefile** points to your root project using **absolute paths** (e.g. `/home/noga/oparating_systems/os_project/...`) to avoid lcov path resolution issues.

* If you move/rename the repo, **edit the `ROOT` line** at the top of `part4/Makefile` to match the new absolute path.
* Optional alternative (make it relocatable): set

  ```make
  ROOT := $(abspath ..)
  ```

  and replace the absolute paths with `$(ROOT)/...`. If you do this, keep `-fprofile-abs-path` in the test compile line to help lcov map files cleanly.

---

## Troubleshooting

* **Coverage page didn’t open automatically**

  * On WSL: `sudo apt install wslu` then rerun, or open manually:

    ```bash
    wslview part4/coverage_html/index.html
    ```
  * On Linux: `xdg-open part4/coverage_html/index.html`
* **lcov “mismatch” or system header warnings**
  These come from libstdc++ internals. We strip `/usr/*` and doctest headers, so they’re safe to ignore.
* **Valgrind not found**
  `sudo apt install valgrind`
* **callgrind\_annotate not found**
  It’s included with Valgrind. Reinstall: `sudo apt install valgrind`

---
