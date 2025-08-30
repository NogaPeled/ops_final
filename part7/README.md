
```markdown
# Part 7 — Graph Algorithms Server (Strategy + Factory)

This part adds a TCP server that accepts *one-line commands* from a client,
builds a graph (random/manual), runs a chosen algorithm via **Strategy**,
and returns a human-readable result. A **Factory** creates the requested
algorithm by name.

Algorithms implemented (names are case-insensitive):
- `MST` — Minimum Spanning Tree weight (Kruskal; **undirected only**)
- `SCC` — Strongly Connected Components count (Kosaraju)
- `MAXFLOW` — Max flow from node `0` to node `n-1` (Edmonds–Karp)
- `HAMILTON` — Hamiltonian circuit existence (backtracking)

## Layout assumptions

Project structure (key files only):

```

os\_project/
├─ include/
│  ├─ graph/Graph.hpp
│  └─ algo/GraphAlgorithm.hpp
├─ src/
│  ├─ graph/Graph.cpp
│  └─ algo/
│     ├─ Euler.cpp
│     └─ AlgorithmFactory.cpp
└─ part7/
├─ Makefile
├─ server.cpp
└─ client.cpp

````

> The `part7/Makefile` automatically points to the project root one directory
> up (`..`) to compile/link with your existing `Graph` and `AlgorithmFactory`.

## Build

From `os_project/part7`:

```bash
make            # builds server and client
# or explicitly
make run-server # builds then runs the server
make run-client # builds then runs the client with CMD=...
````

## Run

Open **two terminals**.

### 1) Start the server

```bash
cd os_project/part7
make run-server
# Output:
# [server] listening on 127.0.0.1:5555
```

Leave it running.

### 2) Send commands from the client

You can use:

* **Convenience targets** (random/manual)
* **Free-form** `CMD='...'`

#### Random graph

```bash
# SCC on a random directed graph of 8 vertices, 12 edges, seed 7:
make run-client-random ALGO=SCC V=8 E=12 SEED=7 DIRECTED=--directed
```

#### Manual graph (0-based vertices)

```bash
# Hamiltonian cycle on an undirected 4-cycle:
make run-client-manual ALGO=HAMILTON V=4 EDGES="0-1 1-2 2-3 3-0"
```

#### Free-form examples

```bash
# SCC on random directed graph:
make run-client CMD='ALG SCC RANDOM 8 12 7 --directed'

# MaxFlow on a directed 4-vertex graph:
make run-client CMD='ALG MAXFLOW MANUAL 4 : 0-1 1-2 2-3 0-2 1-3 --directed'

# MST on undirected graph (note: MST is undefined for directed graphs):
make run-client CMD='ALG MST MANUAL 4 : 0-1 1-2 2-3 3-0'
```

## Protocol (what the server expects)

Each client call sends **one line** ending with `\n`:

```
ALG <MST|SCC|MAXFLOW|HAMILTON> RANDOM <V> <E> <SEED> [--directed]
ALG <MST|SCC|MAXFLOW|HAMILTON> MANUAL <V> : u-v u-v ... [--directed]
```

* Vertices are **0-based** (`0..V-1`).
* For `MANUAL`, edges are separated by spaces.
* Add `--directed` to build a directed graph; otherwise undirected.

## Troubleshooting

* **“Unknown. Use:”** — You probably sent `ALGO` instead of `ALG`,
  or the command is malformed. Use `ALG` exactly.
* **Server not responding** — Make sure it’s running and listening
  on `127.0.0.1:5555` (see server output).
* **MST errors** — MST is only defined for **undirected** graphs.

## Clean

```bash
make clean
```

```
