
# Part 3 — Random Graph + Euler (CLI)

This part builds a small CLI app that:
- parses -v <V> -e <E> -s <SEED> [--directed]
- generates a random graph (undirected by default)
- runs the *Euler circuit* algorithm and prints the result

Your program file is *main.cpp*.

---

## Project layout

os_project/ include/ graph/Graph.hpp algo/Euler.hpp src/ graph/Graph.cpp algo/Euler.cpp part3/ main.cpp Makefile README.md

---

## Build

From the part3/ directory:

```bash
make

This produces: bin/part3_random

Clean:

make clean


---

Run

Direct execution:

./bin/part3_random -v 6 -e 7 -s 1
./bin/part3_random -v 8 -e 10 -s 42 --directed

Makefile helpers (default V=6 E=7 SEED=1):

# Undirected (defaults)
make run

# Override defaults
make run V=10 E=12 SEED=123

# Directed
make run-directed V=8 E=10 SEED=42

Example output

Generated UndirectedGraph(6V,7E)
Euler circuit: 0 -> 3 -> 1 -> 2 -> 0

If no Euler circuit exists, you’ll see a message indicating that.


---

Notes & limits

Vertices are labeled 0..V-1.

No self-loops or parallel edges are added.

Keep E within simple-graph limits:

Undirected: E ≤ V*(V-1)/2

Directed:   E ≤ V*(V-1)


Weights/capacities are generated as 1 for Euler.