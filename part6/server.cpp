// ==================== server.cpp ====================
// TCP server using poll(); handles multiple clients.
// Commands (one line each):
//   RANDOM <V> <E> <SEED> [--directed]
//   MANUAL <V> : u-v u-v ...
//   QUIT
// Builds a Graph, runs Euler, replies with result.
// ====================================================

#include "graph/Graph.hpp"            // Graph class from your project
#include "algo/Euler.hpp"             // Euler algorithm from your project

#include <arpa/inet.h>                // htons, inet_ntop, etc.
#include <netdb.h>                    // getaddrinfo/freeaddrinfo
#include <poll.h>                     // poll(), struct pollfd
#include <sys/socket.h>               // socket/bind/listen/accept/send/recv
#include <unistd.h>                   // close()
#include <csignal>                    // std::signal
#include <cerrno>                     // errno
#include <cstring>                    // std::memset, std::strerror
#include <iostream>                   // std::cout, std::cerr
#include <sstream>                    // std::istringstream, std::ostringstream
#include <string>                     // std::string
#include <vector>                     // std::vector
#include <set>                        // std::set
#include <random>                     // std::mt19937
#include <algorithm>                  // std::minmax

// -------- simple config (no extra header) --------
static constexpr const char* kIP        = "127.0.0.1"; // bind to loopback only
static constexpr const char* kPort      = "5555";      // port as string (for getaddrinfo)
static constexpr int         kBacklog   = 16;          // listen backlog
static constexpr int         kBufSize   = 4096;        // read buffer size
static constexpr int         kNoTimeout = -1;          // poll() wait forever

// We keep all active fds here; index 0 will hold the listening socket.
static std::vector<pollfd> g_fds;      // global so SIGINT handler can close them

// Close and clear all sockets we track.
static void close_all() {
    for (auto& p : g_fds)              // iterate all tracked descriptors
        if (p.fd != -1)                // skip already closed
            close(p.fd);               // close it
    g_fds.clear();                     // empty the vector
}

// SIGINT (Ctrl+C) handler — shut down cleanly.
static void handle_sigint(int) {
    std::cout << "\n[server] SIGINT: shutting down…" << std::endl; // friendly log
    close_all();                         // close all sockets
    std::_Exit(0);                       // exit immediately without unwinding
}

// Build a random graph with exactly E unique edges/arcs (no self-loops).
static Graph make_random_graph(std::size_t V, std::size_t E, unsigned seed, bool directed) {
    Graph::Options opt;                // build options
    opt.allowSelfLoops  = false;       // disallow self-loops
    opt.allowMultiEdges = false;       // disallow duplicates
    Graph g(V, directed ? Graph::Kind::Directed : Graph::Kind::Undirected, opt); // create graph

    std::mt19937 rng(seed);            // PRNG
    std::uniform_int_distribution<int> pick(0, (int)V - 1); // vertex picker

    std::set<std::pair<int,int>> used; // remember edges to avoid duplicates
    std::size_t added = 0;             // how many edges we’ve added

    while (added < E) {                // loop until we have E edges
        int u = pick(rng);             // random u
        int v = pick(rng);             // random v
        if (u == v) continue;          // skip self-loops

        if (directed) {                // for directed graphs
            auto key = std::make_pair(u, v);      // ordered pair (u,v)
            if (used.count(key)) continue;        // skip duplicates
            g.addEdge(u, v, 1);                   // add single arc u->v
            used.insert(key);                     // remember it
        } else {                        // for undirected graphs
            auto mm = std::minmax(u, v);          // canonicalize (small,big)
            if (used.count(mm)) continue;         // skip duplicates
            g.addEdge(u, v, 1);                   // add undirected edge
            used.insert(mm);                      // remember it
        }
        ++added;                                    // count this edge
    }
    return g;                                       // return the graph
}

// Parse a MANUAL command line into 'out' (undirected, 0-based).
// Returns true on success; otherwise sets 'err' and returns false.
static bool parse_manual(const std::string& line, Graph& out, std::string& err) {
    std::istringstream iss(line);       // stream for tokenizing
    std::string mode;                   // first token (should be MANUAL)
    iss >> mode;                        // read mode
    if (mode != "MANUAL") { err = "Expected MANUAL"; return false; } // guard

    std::size_t V = 0; char colon = 0;  // vertex count and colon separator
    iss >> V >> colon;                  // read "<V> :"
    if (V == 0 || colon != ':') {       // validate syntax
        err = "Format: MANUAL <V> : u-v u-v ... (0-based)"; // error text
        return false;                   // fail
    }

    Graph::Options opt;                 // options for the graph
    opt.allowSelfLoops  = false;        // no self-loops
    opt.allowMultiEdges = false;        // no multi-edges
    out = Graph(V, Graph::Kind::Undirected, opt);  // construct result graph

    std::set<std::pair<int,int>> seen;  // track edges to avoid duplicates
    std::string tok;                    // token like "u-v"
    while (iss >> tok) {                // read rest of tokens
        auto dash = tok.find('-');      // locate '-'
        if (dash == std::string::npos) { err = "Bad token: " + tok; return false; } // must have dash
        int u = std::stoi(tok.substr(0, dash));   // parse u
        int v = std::stoi(tok.substr(dash + 1));  // parse v
        if (u < 0 || v < 0 || (std::size_t)u >= V || (std::size_t)v >= V || u == v) { // validate range
            err = "Invalid endpoints in token: " + tok; return false; // invalid
        }
        auto mm = std::minmax(u, v);    // canonical undirected key
        if (seen.count(mm)) { err = "Duplicate edge: " + tok; return false; } // no dupes
        out.addEdge(u, v, 1);           // add the edge
        seen.insert(mm);                // remember it
    }
    return true;                        // success
}

// Turn a graph into a response: label + Euler result.
static std::string run_euler_and_format(const Graph& g) {
    std::ostringstream oss;             // output builder
    oss << "Generated " << g.label() << "\n"; // graph summary
    Euler e;                            // solver object
    oss << e.run(g) << "\n";            // append solver result (message text)
    return oss.str();                   // return string
}

// Send a whole string over a socket (best-effort loop until done or error).
static void send_all(int fd, const std::string& s) {
    const char* p = s.c_str();          // raw pointer to bytes
    std::size_t left = s.size();        // bytes remaining
    while (left) {                      // while not fully sent
        ssize_t n = ::send(fd, p, left, 0); // try to send
        if (n <= 0) return;             // on error or disconnect, stop
        p    += n;                      // advance pointer
        left -= (std::size_t)n;         // reduce remaining
    }
}

// Accept a new client and add it to the poll set.
static void accept_client(int listen_fd) {
    sockaddr_storage addr{};            // peer address storage
    socklen_t alen = sizeof(addr);      // size of storage
    int cfd = ::accept(listen_fd, (sockaddr*)&addr, &alen); // accept()
    if (cfd < 0) { std::perror("accept"); return; }         // log error
    g_fds.push_back({cfd, POLLIN, 0});  // watch it for readability
    std::cout << "[server] client fd=" << cfd << " connected\n"; // log
}

// Handle one line command from a client socket.
static void handle_command(int cfd, const std::string& line) {
    std::istringstream iss(line);       // tokenize line
    std::string cmd;                    // first word: command
    iss >> cmd;                         // read it
    if (cmd == "QUIT") {                // client asks to close
        std::cout << "[server] client fd=" << cfd << " quit\n"; // log
        ::shutdown(cfd, SHUT_RDWR);     // shutdown both ways
        ::close(cfd);                   // close fd
        // mark in g_fds later; we set POLLIN=0 to drop it
        for (auto& p : g_fds)           // find the pollfd
            if (p.fd == cfd) { p.fd = -1; p.events = 0; p.revents = 0; break; } // mark dead
        return;                         // done
    }
    if (cmd == "RANDOM") {              // RANDOM V E SEED [--directed]
        std::size_t V=0, E=0;           // vertex & edge counts
        unsigned seed=0;                 // seed
        std::string flag;               // maybe --directed
        iss >> V >> E >> seed >> flag;  // read numbers + optional flag
        if (V==0) { send_all(cfd, "Error: V must be > 0\n"); return; } // validate
        bool directed = (flag == "--directed"); // detect directed mode
        Graph g = make_random_graph(V, E, seed, directed);  // build random graph
        send_all(cfd, run_euler_and_format(g));             // send result
        return;                                             // done
    }
    if (cmd == "MANUAL") {              // MANUAL <V> : u-v u-v ...
        Graph g;                        // will be filled by parse_manual
        std::string err;                // error message
        if (!parse_manual(line, g, err)) { send_all(cfd, "Error: " + err + "\n"); return; } // parse
        send_all(cfd, run_euler_and_format(g)); // run & reply
        return;                         // done
    }
    // Unknown command → send short help.
    send_all(cfd,
             "Unknown command.\n"
             "Usage:\n"
             "  RANDOM <V> <E> <SEED> [--directed]\n"
             "  MANUAL <V> : u-v u-v ...\n"
             "  QUIT\n");
}

// Read once from client, process a single line command.
static void read_from_client(int idx) {
    pollfd& p = g_fds[idx];             // reference to poll slot
    char buf[kBufSize];                 // buffer for recv
    ssize_t n = ::recv(p.fd, buf, sizeof(buf)-1, 0); // read bytes
    if (n <= 0) {                       // <=0: disconnect or error
        std::cout << "[server] client fd=" << p.fd << " disconnected\n"; // log
        ::close(p.fd);                  // close socket
        p.fd = -1; p.events = 0; p.revents = 0; // mark as dead
        return;                         // done
    }
    buf[n] = '\0';                      // null-terminate to make a C-string
    std::string line(buf);              // convert to std::string
    // trim trailing CR/LF:
    while (!line.empty() && (line.back()=='\n' || line.back()=='\r'))
        line.pop_back();                // drop newline chars
    std::cout << "[server] fd=" << p.fd << " cmd: " << line << "\n"; // log received command
    handle_command(p.fd, line);         // parse + execute command
}

int main() {
    std::signal(SIGINT, handle_sigint); // install Ctrl+C handler

    addrinfo hints{};                   // zero-initialized hints
    hints.ai_family   = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags    = AI_PASSIVE;     // we will bind

    addrinfo* res = nullptr;            // result list head
    if (getaddrinfo(kIP, kPort, &hints, &res) != 0) { // resolve bind address
        std::perror("getaddrinfo");     // print error
        return 1;                       // fail
    }

    int sfd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol); // make socket
    if (sfd < 0) { std::perror("socket"); freeaddrinfo(res); return 1; }    // guard

    int yes = 1;                        // setsockopt value
    ::setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); // allow quick rebinding

    if (::bind(sfd, res->ai_addr, res->ai_addrlen) < 0) { // bind to ip:port
        std::perror("bind");           // log
        ::close(sfd); freeaddrinfo(res); return 1; // cleanup + fail
    }
    if (::listen(sfd, kBacklog) < 0) { // start listening
        std::perror("listen");         // log
        ::close(sfd); freeaddrinfo(res); return 1; // cleanup + fail
    }
    freeaddrinfo(res);                  // free address info (no longer needed)

    g_fds.push_back({sfd, POLLIN, 0});  // watch server socket for new clients
    std::cout << "[server] listening on " << kIP << ":" << kPort << "\n"; // info

    while (true) {                      // main event loop
        int nready = ::poll(g_fds.data(), g_fds.size(), kNoTimeout); // wait for events
        if (nready < 0) {               // poll error
            if (errno == EINTR) continue; // interrupted by signal → continue
            std::perror("poll");        // log other errors
            break;                       // break loop & shutdown
        }
        for (std::size_t i = 0; i < g_fds.size() && nready > 0; ++i) { // scan fds
            pollfd& p = g_fds[i];       // reference
            if (!(p.revents & POLLIN))  // we only care about readable events
                continue;               // skip if not readable
            --nready;                    // count handled event

            if (p.fd == sfd) {          // event on listening socket
                accept_client(sfd);      // accept new client
            } else {                     // event on client socket
                read_from_client((int)i);// read & process one command
            }
        }
        // Compact vector: remove closed entries (fd == -1)
        g_fds.erase(std::remove_if(g_fds.begin(), g_fds.end(),
                                   [](const pollfd& p){ return p.fd == -1; }),
                     g_fds.end());       // erase them
    }

    close_all();                         // ensure sockets are closed
    return 0;                            // done
}
