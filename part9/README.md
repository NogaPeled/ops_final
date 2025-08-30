
````
# Part 9 — Multithreading: Pipeline + Active Objects

This server implements the **Pipeline** pattern with **Active Objects** and runs
**all four algorithms** (MST, SCC, Max Flow, Hamiltonian) for each client request.

It reuses:
- `Graph` (include/graph + src/graph)
- `AlgorithmFactory` (include/algo/GraphAlgorithm.hpp + src/algo/AlgorithmFactory.cpp)
- The **Part-7 client** binary (we compile/reuse it from `../part7/client.cpp`).

## Layout

os_project/
  include/
    graph/Graph.hpp
    algo/GraphAlgorithm.hpp
  src/
    graph/Graph.cpp
    algo/Euler.cpp
    algo/AlgorithmFactory.cpp
  part7/
    client.cpp
  part9/
    server_pipeline.cpp
    Makefile
    README.md


## Build

From `part9/`:

```bash
make          # builds bin/server_pipeline and bin/client
````

Clean:

```bash
make clean
```

## Run

Start server (terminal #1):

```bash
make run-server
# [Pipeline server] listening on 127.0.0.1:5555
```

Send requests (terminal #2):

### Random graphs

```bash
# Undirected
make run-client-random V=6 E=7 SEED=1

# Directed
make run-client-random V=8 E=12 SEED=7 DIRECTED=--directed
```

### Manual graphs (0-based)

```bash
# 4-cycle (Hamiltonian)
make run-client-manual V=4 EDGES="0-1 1-2 2-3 3-0"

# Directed example
make run-client-manual V=4 EDGES="0-1 1-2 2-3 0-2 1-3" DIRECTED=--directed
```

### Free-form

```bash
make run-client CMD='ALG ALL RANDOM 8 12 7 --directed'
make run-client CMD='ALG ALL MANUAL 4 : 0-1 1-2 2-3 3-0'
```

## What you get back

A multi-line response, e.g.:

```
Graph: UndirectedGraph(4V,4E)
MST: MST weight: 3 (edges used: 3).
SCC: SCC count: 1.
MAXFLOW: Max flow (0 -> 3): 2.
HAMILTON: Hamiltonian circuit: 0 -> 1 -> 2 -> 3 -> 0
```

## Concurrency model

* **Active Objects (threads + queues):**

  1. Parse/BuildGraph
  2. Dispatcher (fan-out)
  3. MST worker
  4. SCC worker
  5. MAXFLOW worker
  6. HAMILTON worker
  7. Aggregator (fan-in)
  8. Sender

* Each request flows through the pipeline; the four algorithms run **in parallel** on
  the same immutable `Graph` (shared via `std::shared_ptr`).

* Ctrl+C performs a clean shutdown.

## Troubleshooting

* **`Unknown. Use:`** — Check your command format:

  ```
  ALG ALL RANDOM <V> <E> <SEED> [--directed]
  ALG ALL MANUAL <V> : u-v u-v ... [--directed]
  ```
* **`connect: Connection refused`** — Start the server first (`make run-server`).
* **`bind: Address already in use`** — Another process is using the port. Stop it or wait \~30s.
* Client path different? Edit `CLIENT_SRCS` in the Makefile.

```

---

```
