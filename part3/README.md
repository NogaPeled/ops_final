
---

### `part3/README-Part3.md`

```markdown
# Part 3 — Random Graph + Euler (undirected or directed)

This program builds a random graph and runs the Euler algorithm on it.  
It supports both **undirected** and **directed** graphs.

## CLI

```

./bin/part3 -v <vertices> -e <edges> -s <seed> \[--directed]

````

- `-v <vertices>`: number of vertices (1..N)
- `-e <edges>`: number of edges (undirected) or arcs (directed)
- `-s <seed>`: seed for the PRNG (reproducible runs)
- `--directed`: build a directed graph instead of undirected

The program prints a one-line summary of the graph and either:
- an Euler circuit in vertex order, or
- a short reason why an Euler circuit doesn’t exist (e.g., *odd degree*, *in/out mismatch*, *not strongly connected*, or *no edges*).

## Build & run (from this folder)

```bash
cd part3
make          # builds ./bin/part3
./bin/part3 -v 8 -e 12 -s 1
./bin/part3 -v 8 -e 20 -s 1 --directed
````

You can also use helper targets:

```bash
make run            # undirected sample: -v 8 -e 12 -s 1
make run-directed   # directed sample:   -v 8 -e 20 -s 1 --directed
```

> This Makefile compiles the core sources from the **project root** using a
> relocatable path (`ROOT := $(abspath ..)`), so you can move the repo and it
> still works.

## Notes / Tips

* **Undirected Euler** requires all vertex degrees to be even and the graph
  connected on the non-isolated vertices.
* **Directed Euler** requires for every vertex `in_degree == out_degree` and strong connectivity on the non-isolated vertices.
* Random graphs may or may not satisfy these—it’s normal to sometimes get
  “No Euler circuit…”. Re-run with a different `-s`, or adjust `-e`.
* The generator avoids self-loops and duplicate edges/arcs based on your Graph
  options; the Euler algorithm works for both graph kinds.

## Troubleshooting

* If you get “No Euler circuit…” it’s usually because the random graph didn’t
  meet the degree/connectivity requirements; increase `-e` or try a new `-s`.
* If compile paths break after moving the repo, this folder’s `Makefile`
  uses `ROOT := $(abspath ..)`. That should auto-fix paths. If not, open the
  Makefile and confirm `ROOT` points one directory up to your project root.

````

---

### `part3/Makefile`

```make
# -------- Part 3 local Makefile (relocatable) --------

CXX   := g++
STD   := -std=c++17
WARN  := -Wall -Wextra -Wpedantic
OPT   := -O2 -g

# Absolute path to project root (one level up from part3/)
ROOT  := $(abspath ..)

INCS  := -I$(ROOT)/include
SRC   := $(ROOT)/src/graph/Graph.cpp \
         $(ROOT)/src/algo/Euler.cpp \
         main.cpp

BIN       := bin
RUN_BIN   := $(BIN)/part3

.PHONY: all run run-directed clean

all: $(RUN_BIN)

$(BIN):
	mkdir -p "$@"

$(RUN_BIN): $(BIN) $(SRC)
	$(CXX) $(STD) $(WARN) $(OPT) $(INCS) $(SRC) -o "$@"

# Quick demos
run: $(RUN_BIN)
	"$(RUN_BIN)" -v 8 -e 12 -s 1

run-directed: $(RUN_BIN)
	"$(RUN_BIN)" -v 8 -e 20 -s 1 --directed

clean:
	rm -rf "$(BIN)"
````
