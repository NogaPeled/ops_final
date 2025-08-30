// ======================= server_lf.cpp (Part 8) =======================   // file banner (for clarity)
// Multithreaded TCP server using the Leader–Followers pattern.              // overall description
// - One listening socket on 127.0.0.1:5555                                  // listen address
// - A pool of threads; exactly one thread is the "leader" at a time.        // LF core idea
//   The leader blocks on accept(); once it accepts a client socket,          // leader behavior
//   it immediately promotes a follower to be the new leader and then         // promotion step
//   *processes* the client (reads one command, builds a graph, and           // worker step
//   runs *all four* algorithms from Part 7), sends a combined reply,         // response
//   and closes the client socket.                                            // lifecycle
// - Commands (one line, newline-terminated):                                 // protocol
//     ALG ALL RANDOM <V> <E> <SEED> [--directed]                             // random graph
//     ALG ALL MANUAL <V> : u-v u-v ... [--directed]                          // manual graph
//   (Reuses your Part 7 algorithms via AlgorithmFactory)                     // reuse note
// =====================================================================     // end banner

#include "graph/Graph.hpp"             // your Graph API (already in /include + /src)
#include "algo/GraphAlgorithm.hpp"     // IGraphAlgorithm + AlgorithmFactory (Part 7)

#include <arpa/inet.h>                 // inet_pton, htons
#include <netdb.h>                     // getaddrinfo, freeaddrinfo
#include <sys/socket.h>                // socket, bind, listen, accept, send, recv
#include <unistd.h>                    // close, shutdown

#include <algorithm>                   // std::minmax
#include <atomic>                      // std::atomic<bool>
#include <condition_variable>          // std::condition_variable
#include <csignal>                     // std::signal
#include <cstring>                     // std::strerror
#include <iostream>                    // std::cout, std::cerr
#include <mutex>                       // std::mutex, std::unique_lock
#include <random>                      // std::mt19937
#include <set>                         // std::set
#include <sstream>                     // std::istringstream, std::ostringstream
#include <string>                      // std::string
#include <thread>                      // std::thread
#include <vector>                      // std::vector

// -------- basic config --------                                                     // small config block header
static constexpr const char* kIP   = "127.0.0.1";                                     // IPv4 loopback to bind
static constexpr const char* kPort = "5555";                                          // TCP port (string for getaddrinfo)
static constexpr int         kBacklog = 32;                                           // listen backlog (pending queue size)
static constexpr int         kBufSz   = 4096;                                         // receive buffer size
static constexpr unsigned    kDefaultThreads = 4;                                     // default thread pool size cap

// -------- leader-followers shared state --------                                    // LF shared state header
static int g_listen_fd = -1;                    // listening socket FD (global so all threads can see)
static std::mutex g_mu;                         // protects leader election flag (g_has_leader)
static std::condition_variable g_cv;            // followers wait on this when no leadership available
static bool g_has_leader = false;               // true if a thread currently holds leadership
static std::atomic<bool> g_stop{false};         // set to true on shutdown (SIGINT), threads exit loops

// Close listening socket (idempotent).                                              // helper to close listen fd safely
static void close_listen_fd() {
    if (g_listen_fd >= 0) {                    // check if socket is valid
        ::close(g_listen_fd);                  // close it
        g_listen_fd = -1;                      // mark as closed
    }
}

// SIGINT handler: stop the server and wake all threads.                              // Ctrl+C handler
static void on_sigint(int) {
    g_stop.store(true);                        // signal stop to all worker loops
    close_listen_fd();                         // close listening socket → accept() will fail
    g_cv.notify_all();                         // wake any waiting followers to observe stop
}

// Lowercase helper.                                                                  // convenience to normalize tokens
static std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c); // lowercase each byte safely
    return s;                                                   // return normalized string
}

// ---------------- Graph builders (same semantics as part 7) ----------------        // graph construction header
static Graph make_random_graph(std::size_t V, std::size_t E, unsigned seed, bool directed) {
    Graph::Options opt; opt.allowSelfLoops=false; opt.allowMultiEdges=false;         // disallow self-loops & duplicates
    Graph g(V, directed? Graph::Kind::Directed : Graph::Kind::Undirected, opt);     // construct graph with kind

    std::mt19937 rng(seed);                                                          // PRNG seeded
    std::uniform_int_distribution<int> pick(0, (int)V-1);                            // vertex picker [0..V-1]
    std::set<std::pair<int,int>> seen;                                              // set to deduplicate edges
    std::size_t added=0;                                                             // count edges added

    while (!g_stop.load() && added < E) {                                            // loop until E edges or stop
        int u = pick(rng), v = pick(rng);                                            // pick two endpoints
        if (u == v) continue;                                                        // skip self-loop
        if (directed) {                                                              // directed case
            auto key = std::make_pair(u,v);                                          // arc key (ordered)
            if (seen.count(key)) continue;                                           // skip duplicates
            g.addEdge(u,v,1);                                                        // add arc with weight 1
            seen.insert(key);                                                        // remember
        } else {                                                                      // undirected case
            auto mm = std::minmax(u,v);                                              // canonical undirected key
            if (seen.count(mm)) continue;                                            // skip duplicates
            g.addEdge(u,v,1);                                                        // add edge with weight 1
            seen.insert(mm);                                                         // remember
        }
        ++added;                                                                     // bump count
    }
    return g;                                                                        // return built graph
}

// Parse: "ALG ALL MANUAL <V> : u-v u-v ... [--directed]"                            // parser signature
static bool parse_manual_all(const std::string& line, Graph& out, std::string& err) {
    std::istringstream iss(line);                                                    // tokenize input line
    std::string kw1, kw2, mode;              // ALG ALL MANUAL                       // placeholders
    iss >> kw1 >> kw2 >> mode;                                                      // read first three tokens
    if (lower(kw1)!="alg" || lower(kw2)!="all" || lower(mode)!="manual") {          // validate header
        err = "Expected: ALG ALL MANUAL <V> : u-v u-v ... [--directed]";            // error message
        return false;                                                                // fail
    }

    std::size_t V=0; char colon=0; iss >> V >> colon;                                // read vertex count and ':'
    if (V==0 || colon!=':') {                                                        // validate
        err = "Format: ALG ALL MANUAL <V> : u-v u-v ... [--directed]";              // error message
        return false;                                                                // fail
    }

    // Slurp the remainder to detect optional --directed at the end.                 // gather remaining tokens
    std::vector<std::string> toks;                                                   // tokens container
    std::string tok;                                                                 // temp token
    while (iss >> tok) toks.push_back(tok);                                          // collect tokens

    bool directed=false;                                                             // default undirected
    if (!toks.empty() && toks.back()=="--directed") { directed=true; toks.pop_back(); } // detect and remove flag

    Graph::Options opt; opt.allowSelfLoops=false; opt.allowMultiEdges=false;         // graph options
    out = Graph(V, directed? Graph::Kind::Directed : Graph::Kind::Undirected, opt);  // construct output graph

    std::set<std::pair<int,int>> seen;                                               // dedupe set
    for (auto& t : toks) {                                                           // parse each edge token
        auto dash = t.find('-');                                                     // find '-'
        if (dash == std::string::npos) { err = "Bad token: " + t; return false; }    // must exist
        int u = std::stoi(t.substr(0,dash));                                         // parse left side
        int v = std::stoi(t.substr(dash+1));                                         // parse right side
        if (u<0 || v<0 || (std::size_t)u>=V || (std::size_t)v>=V || u==v) {          // bounds & no self-loop
            err = "Invalid endpoints in token: " + t; return false;                  // error
        }
        if (out.directed()) {                                                        // directed
            auto key = std::make_pair(u,v);                                          // arc key
            if (seen.count(key)) { err = "Duplicate arc: " + t; return false; }      // dup check
            seen.insert(key); out.addEdge(u,v,1);                                     // add arc
        } else {                                                                      // undirected
            auto mm = std::minmax(u,v);                                              // canonical edge key
            if (seen.count(mm)) { err = "Duplicate edge: " + t; return false; }      // dup check
            seen.insert(mm); out.addEdge(u,v,1);                                      // add edge
        }
    }
    return true;                                                                      // parsed OK
}

// Build graph from either RANDOM or MANUAL "ALG ALL ..." command.                    // dispatcher for building
static bool build_graph_from_command(const std::string& line, Graph& out, std::string& err) {
    std::istringstream iss(line);                                                    // tokenize line
    std::string kw1, kw2, mode; iss >> kw1 >> kw2 >> mode;                           // read header

    if (lower(kw1)!="alg" || lower(kw2)!="all") {                                    // not our command family
        err = "Unknown. Use:\n"                                                      // help text
              "  ALG ALL RANDOM <V> <E> <SEED> [--directed]\n"
              "  ALG ALL MANUAL <V> : u-v u-v ... [--directed]\n";
        return false;                                                                // fail
    }

    if (lower(mode) == "random") {                                                   // RANDOM path
        std::size_t V=0, E=0; unsigned seed=0; std::string flag;                     // input params
        iss >> V >> E >> seed >> flag;                                               // parse
        if (V==0) { err="V must be > 0"; return false; }                             // validate
        const bool directed = (flag=="--directed");                                  // detect flag
        out = make_random_graph(V, E, seed, directed);                                // build
        return true;                                                                  // success
    }

    if (lower(mode) == "manual") {                                                   // MANUAL path
        return parse_manual_all(line, out, err);                                      // delegate
    }

    err = "Bad mode. Use RANDOM or MANUAL.";                                         // unknown mode
    return false;                                                                     // fail
}

// Run all four algorithms via the Part 7 factory and format the combined reply.      // multi-algorithm runner
static std::string run_all_algorithms(const Graph& g) {
    std::ostringstream out;                                                          // build response
    out << "Graph: " << g.label() << "\n";                                           // include graph label

    auto do_one = [&](const char* name){                                             // helper to run single algo
        auto alg = AlgorithmFactory::create(name);                                    // create strategy by name
        if (alg) out << name << ": " << alg->run(g) << "\n";                          // append result
        else     out << name << ": (unavailable)\n";                                  // safety: if not linked
    };

    // Names expected by your Part 7 factory: "mst", "scc", "maxflow", "hamilton"     // supported set
    do_one("MST");                                                                    // MST weight
    do_one("SCC");                                                                    // SCC count
    do_one("MAXFLOW");                                                                // Max flow 0→n-1
    do_one("HAMILTON");                                                               // Hamiltonian circuit

    return out.str();                                                                 // return full reply
}

// Send the whole string (best-effort loop).                                          // robust send utility
static void send_all(int fd, const std::string& s) {
    const char* p = s.c_str();                                                       // byte pointer
    std::size_t left = s.size();                                                     // bytes remaining
    while (left) {                                                                    // loop until sent
        ssize_t n = ::send(fd, p, left, 0);                                          // try to send
        if (n <= 0) return;                                                          // error/closed → give up
        p    += n;                                                                   // advance pointer
        left -= (std::size_t)n;                                                      // reduce remaining
    }
}

// Handle one connected client socket: read one line, build graph, run all, reply, close. // per-client handler
static void handle_client(int cfd) {
    char buf[kBufSz];                                                                 // receive buffer
    ssize_t n = ::recv(cfd, buf, sizeof(buf)-1, 0);                                   // read once (single-line protocol)
    if (n <= 0) { ::close(cfd); return; }                                             // on error/EOF → close & return
    buf[n] = '\0';                                                                    // terminate as C-string

    std::string line(buf);                                                            // convert to std::string
    // Trim trailing CR/LF                                                                  // remove newline
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))            // while last is NL/CR
        line.pop_back();                                                              // pop it

    Graph g; std::string err;                                                         // output graph & error
    if (!build_graph_from_command(line, g, err)) {                                    // build graph per command
        send_all(cfd, "Error: " + err + "\n");                                        // send error back
        ::close(cfd);                                                                 // close client
        return;                                                                       // done
    }

    const std::string reply = run_all_algorithms(g);                                  // run all strategies
    send_all(cfd, reply);                                                             // send combined result
    ::shutdown(cfd, SHUT_RDWR);                                                       // graceful shutdown
    ::close(cfd);                                                                     // close socket
}

// Leader–Followers thread body.                                                      // worker thread function
static void worker_thread() {
    while (!g_stop.load()) {                                                          // loop until stop
        // ---- Become leader ----                                                     // leader election section
        {
            std::unique_lock<std::mutex> lk(g_mu);                                    // lock mutex
            // Wait until no leader is present or stop requested                      // wait predicate
            g_cv.wait(lk, []{ return g_stop.load() || !g_has_leader; });              // block until can lead
            if (g_stop.load()) return;                                                // exit if stopping
            g_has_leader = true; // I'm the leader now                                // acquire leadership
        }

        // ---- Leader blocks on accept() ----                                         // accept new client
        sockaddr_storage addr{};                                                      // peer address
        socklen_t alen = sizeof(addr);                                               // address length
        int cfd = ::accept(g_listen_fd, (sockaddr*)&addr, &alen);                     // blocking accept

        // ---- Immediately promote next follower to be the new leader ----            // leadership handoff
        {
            std::lock_guard<std::mutex> lk(g_mu);                                     // lock to modify flag
            g_has_leader = false;      // release leadership                           // mark no leader
            g_cv.notify_one();         // wake one follower as next leader             // wake next
        }

        // If accept failed (e.g., listening socket closed on shutdown), exit.         // handle accept failure
        if (cfd < 0) {
            if (g_stop.load()) return;                                                // stopping → exit
            // transient error: continue loop to try again                             // otherwise retry
            continue;
        }

        // ---- Process this client (now as a worker) ----                             // worker phase
        handle_client(cfd);                                                           // handle connected client
    }
}

// Setup listening socket.                                                            // create/bind/listen helper
static bool setup_listen_socket() {
    addrinfo hints{}; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM; hints.ai_flags = AI_PASSIVE; // hint config
    addrinfo* res = nullptr;                                                         // result list
    if (getaddrinfo(kIP, kPort, &hints, &res) != 0) {                                // resolve bind addr
        std::perror("getaddrinfo");                                                  // log error
        return false;                                                                // fail
    }

    g_listen_fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);      // create socket
    if (g_listen_fd < 0) {
        std::perror("socket");                                                       // log error
        freeaddrinfo(res);                                                           // cleanup
        return false;                                                                // fail
    }

    int yes = 1;                                                                     // reuseaddr flag
    ::setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));          // allow quick rebind

    if (::bind(g_listen_fd, res->ai_addr, res->ai_addrlen) < 0) {                    // bind to ip:port
        std::perror("bind");                                                         // log
        freeaddrinfo(res);                                                           // cleanup hints
        close_listen_fd();                                                           // close socket
        return false;                                                                // fail
    }
    if (::listen(g_listen_fd, kBacklog) < 0) {                                       // start listening
        std::perror("listen");                                                       // log
        freeaddrinfo(res);                                                           // cleanup hints
        close_listen_fd();                                                           // close socket
        return false;                                                                // fail
    }
    freeaddrinfo(res);                                                               // free addrinfo
    return true;                                                                     // success
}

int main() {
    std::signal(SIGINT, on_sigint);                                                  // register Ctrl+C handler

    if (!setup_listen_socket()) return 1;                                            // setup listening socket
    std::cout << "[LF server] listening on " << kIP << ":" << kPort << "\n";         // log startup

    // Start thread pool.                                                             // pool creation
    const unsigned nThreads =
        std::max(2u, std::min(kDefaultThreads, (unsigned)std::thread::hardware_concurrency())); // choose pool size
    std::vector<std::thread> pool;                                                   // thread container
    pool.reserve(nThreads);                                                          // reserve capacity
    for (unsigned i=0;i<nThreads;++i) pool.emplace_back(worker_thread);              // spawn workers

    // Initially, there is no leader → wake one follower to become leader.            // kickstart leadership
    {
        std::lock_guard<std::mutex> lk(g_mu);                                        // lock for flag
        g_has_leader = false;                                                        // ensure no leader set
        g_cv.notify_one();                                                           // wake one to lead
    }

    // Join threads (until SIGINT).                                                   // wait for workers to finish
    for (auto& t : pool) t.join();                                                   // join each thread

    close_listen_fd();                                                               // ensure listener is closed
    return 0;                                                                        // normal exit
}
