
---

# Part 8 — Leader–Followers (LF) Multithreaded Server

This part adds a **multithreaded TCP server** using the **Leader–Followers** pattern that:

* Listens on `127.0.0.1:5555`
* Accepts a **single-line** request from each client
* Builds a graph (random or manual)
* Runs **all four algorithms** from Part 7 (**MST**, **SCC**, **Max Flow**, **Hamiltonian**)
* Sends a combined reply and closes the connection

It reuses your project’s `Graph` plus the Part-7 Strategy/Factory (`AlgorithmFactory`).

---

## Directory layout (expected)

```
os_project/
  include/
    graph/Graph.hpp
    algo/GraphAlgorithm.hpp
  src/
    graph/Graph.cpp
    algo/Euler.cpp
    algo/AlgorithmFactory.cpp
  part7/
    client.cpp                # you can reuse this client
  part8/
    server_lf.cpp             # LF server (this part)
    Makefile                  # provided
    client.cpp (optional)     # if you copy the client here
```

> The **Makefile** in `part8/` will:
>
> * Prefer a local `part8/client.cpp`; if not present, it will automatically use `part7/client.cpp`.
> * Prefer `server_lf.cpp`; if not present it will try `server.cpp`.

You can also override the client path explicitly:

```bash
make run-client-random CLIENT_MAIN=/absolute/path/to/client.cpp V=8 E=12 SEED=7
```

---

## Build

From `part8/`:

```bash
make          # builds bin/server_lf and bin/client
```

Clean:

```bash
make clean
```

---

## Run

### 1) Start the server (terminal #1)

```bash
make run-server
```

Expected:

```
[LF server] listening on 127.0.0.1:5555
```

### 2) Send requests with the client (terminal #2)

The LF server always runs **all four algorithms**. Commands start with:

```
ALG ALL ...
```

#### Quick helpers

* **Random (undirected)**

  ```bash
  make run-client-random V=6 E=7 SEED=1
  ```

* **Random (directed)**

  ```bash
  make run-client-random V=8 E=12 SEED=7 DIRECTED=--directed
  ```

* **Manual (undirected)**

  ```bash
  # 4-cycle → has a Hamiltonian circuit
  make run-client-manual V=4 EDGES="0-1 1-2 2-3 3-0"
  ```

* **Manual (directed)**

  ```bash
  make run-client-manual V=4 EDGES="0-1 1-2 2-3 0-2 1-3" DIRECTED=--directed
  ```

#### Free-form command

You can pass the whole command yourself:

```bash
make run-client CMD='ALG ALL RANDOM 8 12 7 --directed'
make run-client CMD='ALG ALL MANUAL 4 : 0-1 1-2 2-3 3-0'
```

---

## Examples (typical output)

**Random directed:**

```
$ make run-client CMD='ALG ALL RANDOM 8 12 7 --directed'
Graph: DirectedGraph(8V,12E)
MST: MST undefined for directed graphs.
SCC: SCC count: 2.
MAXFLOW: Max flow (0 -> 7): 3.
HAMILTON: No Hamiltonian circuit.
```

**Manual undirected 4-cycle:**

```
$ make run-client-manual V=4 EDGES="0-1 1-2 2-3 3-0"
Graph: UndirectedGraph(4V,4E)
MST: MST weight: 4 (edges used: 3).
SCC: SCC count: 1.
MAXFLOW: Max flow (0 -> 3): 2.
HAMILTON: Hamiltonian circuit: 0 -> 1 -> 2 -> 3 -> 0
```

Notes:

* Vertices are **0-based** in manual mode.
* `--directed` is optional and, when used, must be at the **end** of the line.
* Unweighted edges are treated as capacity/weight **1**.
* MST is **undefined** for directed graphs (you’ll see a message).

---

## Concurrency test (Leader–Followers)

With the server running, fire multiple clients in parallel:

```bash
printf 'ALG ALL RANDOM 8 12 7 --directed\n%.0s' {1..10} \
  | xargs -I{} -P 10 ./bin/client {}
```

You’ll see interleaved replies. Internally, exactly one thread is the **leader** (blocking on `accept()`), promotes the next follower, then processes its own client.

---

## Troubleshooting

* **“Unknown. Use:”**
  Your line didn’t match one of:

  ```
  ALG ALL RANDOM <V> <E> <SEED> [--directed]
  ALG ALL MANUAL <V> : u-v u-v ... [--directed]
  ```

* **`connect: Connection refused`**
  Start the server first: `make run-server`.

* **`bind: Address already in use`**
  Another process is using `127.0.0.1:5555`. Stop it or change the port in `server_lf.cpp`.

* **Client build fails**
  Ensure you have `part8/client.cpp` **or** `part7/client.cpp`.
  Or point to a client explicitly:

  ```bash
  make run-client-random CLIENT_MAIN=/full/path/to/client.cpp V=8 E=12 SEED=7
  ```

---

## What this part fulfills

* **8(a)** Multithreaded server — implemented via **Leader–Followers** thread pool
* **8(b)** For each request, runs **all four algorithms** from Part 7 and returns a single combined answer

---
