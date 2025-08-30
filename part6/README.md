
````markdown
# Part 6 — Networked Euler Server & Client

This part adds a small TCP server that accepts graph requests from multiple clients,
builds the graph (random or manual), runs the Euler-circuit check, and returns a
human-readable result.

- **Server**: `part6/server.cpp`
- **Client**: `part6/client.cpp`
- **Port**: `5555` (IPv4, localhost)
- **Protocol**: simple one-line text commands (see below)
- **Concurrency**: handled with `poll()` (multiple clients at once)
- **Graceful shutdown**: `Ctrl+C` (SIGINT) closes all sockets cleanly

---

## Build

From inside `part6/`:

```bash
make            # builds bin/server and bin/client
````

> Requires `g++` and `make`. (On WSL/Ubuntu: `sudo apt install build-essential`)

---

## Run

### 1) Start the server

```bash
make run-server
# or:
./bin/server
```

You should see:

```
[Server listening on 127.0.0.1:5555]
```

Server will keep running. Stop with `Ctrl+C`.

### 2) Use the client

The client sends one command to the server and prints the response.

Two modes are supported:

#### RANDOM

Create a random undirected graph with **V** vertices and **E** edges, using **SEED**
for reproducibility (no self-loops or multi-edges).

```bash
# target provided in the Makefile:
make run-client-random
# or directly:
./bin/client RANDOM 8 12 1
#            ^mode  ^V ^E ^SEED
```

#### MANUAL

Send your own undirected graph. Format:

```
MANUAL <V> : u-v u-v u-v ...
```

* Vertices are **0..V-1**
* Edges are space-separated pairs written as `u-v`
* No duplicates, no self-loops

Examples:

```bash
# a 4-cycle (Eulerian)
./bin/client MANUAL 4 : 0-1 1-2 2-3 3-0

# a 5-cycle (Eulerian)
./bin/client MANUAL 5 : 0-1 1-2 2-3 3-4 4-0

# not Eulerian (odd degrees)
./bin/client MANUAL 3 : 0-1 1-2
```

---

## Protocol (one line per request)

```
RANDOM <V> <E> <SEED>\n
MANUAL <V> : u-v u-v u-v ...\n
```

**Server response** is a readable text block containing either an Euler circuit
(e.g., `Euler circuit: 0 -> 1 -> 2 -> 3 -> 0`) or a diagnostic message
(e.g., `No Euler circuit: at least one vertex has odd degree.`).
If the graph has no edges, you’ll see `Graph has no edges; trivial Euler circuit at vertex 0.`

---

## What’s implemented

* TCP server (IPv4) on `127.0.0.1:5555`
* `poll()`-based loop (accepts/handles multiple clients)
* Robust parsing and input validation (bad inputs return an error line)
* Random graph generation that avoids self-loops & parallel edges
* Euler algorithm shared from `src/algo/Euler.cpp` + `src/graph/Graph.cpp`
* Clean SIGINT handling: closes all sockets and exits

---

## Helpful Make targets

From `part6/`:

```bash
make             # build both binaries
make run-server  # run ./bin/server
make run-client-random
make clean       # remove ./bin and ./artifacts (if used)
```

You can also call the client directly with your own arguments.

---

## Troubleshooting

* **“bind: Address already in use”**
  Another process is using port 5555. Stop it, or change the port in the code
  (both server & client).

* **No response**
  Ensure the server is running first. The client connects to `127.0.0.1:5555`.

* **Windows/WSL**
  Run both server and client inside WSL for simplicity (they use `127.0.0.1`).

---

## Extending (optional ideas)

* Add a `DIRECTED` mode and reuse the directed Euler checker.
* Support JSON for the request/response payload.
* Add logging to a file in `part6/artifacts/`.

---

```
