// ==================== server.cpp (part 7) ====================
// TCP server using poll(); accepts one-line algorithm requests:
//   ALG <MST|SCC|MAXFLOW|HAMILTON> RANDOM <V> <E> <SEED> [--directed]
//   ALG <MST|SCC|MAXFLOW|HAMILTON> MANUAL <V> : u-v u-v ... [--directed]
// Replies with a human-readable result string from the chosen strategy.
// =============================================================

#include "../include/graph/Graph.hpp"                    // Graph from your project
#include "../include/algo/GraphAlgorithm.hpp"      // IGraphAlgorithm interface
// We include the factory & strategy implementations by linking AlgorithmFactory.cpp
// (either compile it into an object or list it in your Makefile).

#include <arpa/inet.h>                        // htons, inet_ntop, inet_pton
#include <netdb.h>                            // getaddrinfo/freeaddrinfo
#include <poll.h>                             // poll(), struct pollfd
#include <sys/socket.h>                       // socket/bind/listen/accept
#include <unistd.h>                           // close(), read(), write()

#include <algorithm>                          // remove_if, minmax
#include <cctype>                             // std::tolower
#include <csignal>                            // std::signal
#include <iostream>                           // std::cout, std::cerr
#include <random>                             // std::mt19937
#include <set>                                // std::set
#include <sstream>                            // std::(i/o)stringstream
#include <string>                             // std::string
#include <vector>                             // std::vector

// ---------- simple config ----------
static constexpr const char* kIP        = "127.0.0.1"; // bind address (loopback)
static constexpr const char* kPort      = "5555";      // TCP port (string)
static constexpr int         kBacklog   = 16;          // listen backlog
static constexpr int         kBufSize   = 4096;        // I/O buffer size
static constexpr int         kNoTimeout = -1;          // poll timeout (-1 = infinite)

// Keep active sockets here; index 0 is the listening socket.
static std::vector<pollfd> g_fds;                      // global so signal handler can close

// --------- tiny helpers ---------
static std::string lower(std::string s){               // lower-case helper
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}
static void send_all(int fd, const std::string& s) {   // send entire string (best effort)
    const char* p = s.c_str(); std::size_t left = s.size();
    while (left) { ssize_t n = ::send(fd, p, left, 0); if (n <= 0) return; p += n; left -= (std::size_t)n; }
}
static void close_all() {                               // close all tracked fds
    for (auto& p : g_fds) if (p.fd != -1) ::close(p.fd);
    g_fds.clear();
}
static void on_sigint(int){                             // SIGINT handler
    std::cout << "\n[server] SIGINT -> shutdown\n";
    close_all();
    std::_Exit(0);
}

// ---------- Build a random graph with E unique edges (no self-loops). ----------
static Graph make_random_graph(std::size_t V, std::size_t E, unsigned seed, bool directed) {
    Graph::Options opt; opt.allowSelfLoops = false; opt.allowMultiEdges = false;       // options
    Graph g(V, directed ? Graph::Kind::Directed : Graph::Kind::Undirected, opt);       // graph
    std::mt19937 rng(seed);                                                             // PRNG
    std::uniform_int_distribution<int> pick(0, (int)V - 1);                             // vertex picker
    std::set<std::pair<int,int>> used;                                                  // dedupe edges/arcs
    std::size_t added = 0;                                                              // counter
    while (added < E) {                                                                 // until E edges
        int u = pick(rng), v = pick(rng);                                              // random endpoints
        if (u == v) continue;                                                           // skip self-loop
        if (directed) {                                                                 // directed case
            auto key = std::make_pair(u, v);                                           // ordered arc
            if (used.count(key)) continue;                                              // dedupe
            g.addEdge(u, v, 1);                                                         // add arc
            used.insert(key);                                                           // remember
        } else {                                                                        // undirected
            auto mm = std::minmax(u, v);                                               // canonical pair
            if (used.count(mm)) continue;                                               // dedupe
            g.addEdge(u, v, 1);                                                         // add edge
            used.insert(mm);                                                            // remember
        }
        ++added;                                                                         // count
    }
    return g;                                                                            // done
}

// ---------- Parse a MANUAL line into a Graph. Format:
// ALG <name> MANUAL <V> : u-v u-v ... [--directed] ----------

static bool parse_manual_line(const std::string& line, Graph& out, std::string& err) {
    std::istringstream iss(line);                                // tokenize line
    std::string kw, name, mode; iss >> kw >> name >> mode;       // read ALG, <name>, MANUAL
    if (lower(kw) != "alg" || lower(mode) != "manual") {         // syntax guard
        err = "Expected: ALG <name> MANUAL <V> : u-v u-v ... [--directed]";
        return false;
    }
    std::size_t V = 0; char colon = 0; iss >> V >> colon;        // read vertex count and colon
    if (V == 0 || colon != ':') {                                 // validate
        err = "Format: ALG <name> MANUAL <V> : u-v u-v ... [--directed]";
        return false;
    }
    std::vector<std::string> toks;                                // collect the rest tokens
    std::string t; while (iss >> t) toks.push_back(t);            // slurp tokens

    bool directed = false;                                        // default undirected
    if (!toks.empty() && toks.back() == "--directed") {           // optional flag at end
        directed = true;                                          // directed mode
        toks.pop_back();                                          // remove flag
    }

    Graph::Options opt; opt.allowSelfLoops=false; opt.allowMultiEdges=false;            // graph opts
    out = Graph(V, directed ? Graph::Kind::Directed : Graph::Kind::Undirected, opt);    // construct

    std::set<std::pair<int,int>> seen;                            // avoid duplicates
    for (const auto& s : toks) {                                  // for each "u-v"
        auto dash = s.find('-');                                  // find '-'
        if (dash == std::string::npos) { err = "Bad token: " + s; return false; }       // must have '-'
        int u = std::stoi(s.substr(0, dash));                     // parse u
        int v = std::stoi(s.substr(dash + 1));                    // parse v
        if (u < 0 || v < 0 || (std::size_t)u >= V || (std::size_t)v >= V || u == v) {   // bounds check
            err = "Invalid endpoints in token: " + s; return false;                     // fail
        }
        if (directed) {                                           // directed: keep order
            auto key = std::make_pair(u, v);                      // ordered arc
            if (seen.count(key)) { err = "Duplicate arc: " + s; return false; }         // dup
            seen.insert(key);                                     // remember
            out.addEdge(u, v, 1);                                 // add arc
        } else {                                                  // undirected
            auto mm = std::minmax(u, v);                          // canonical pair
            if (seen.count(mm)) { err = "Duplicate edge: " + s; return false; }         // dup
            seen.insert(mm);                                      // remember
            out.addEdge(u, v, 1);                                 // add edge
        }
    }
    return true;                                                  // success
}

// ---------- Run the chosen algorithm and format a response ----------
static std::string run_and_format(const std::string& name, const Graph& g) {
    auto algo = AlgorithmFactory::create(name);                   // strategy from factory
    if (!algo) return "Unknown algorithm.\n";                     // guard
    std::ostringstream oss;                                       // result builder
    oss << "Graph: " << g.label() << "\n";                        // prefix with graph label
    oss << algo->run(g) << "\n";                                  // algorithm result
    return oss.str();                                              // done
}

// ---------- Command handler: parses one line request ----------
static void handle_command(int cfd, const std::string& line) {
    std::istringstream iss(line);                                 // tokenize
    std::string kw; iss >> kw;                                    // first word
    if (lower(kw) != "alg") {                                     // must start with ALG
        send_all(cfd,
            "Unknown. Use:\n"
            "  ALG <MST|SCC|MAXFLOW|HAMILTON> RANDOM <V> <E> <SEED> [--directed]\n"
            "  ALG <MST|SCC|MAXFLOW|HAMILTON> MANUAL <V> : u-v u-v ... [--directed]\n");
        return;                                                    // bail
    }

    std::string name, mode; iss >> name >> mode;                  // read algo name + mode
    if (lower(mode) == "random") {                                // RANDOM branch
        std::size_t V=0, E=0; unsigned seed=0; std::string flag;  // holders
        iss >> V >> E >> seed >> flag;                            // read numbers + maybe flag
        bool directed = (flag == "--directed");                   // detect directed
        Graph g = make_random_graph(V, E, seed, directed);        // build random graph
        send_all(cfd, run_and_format(name, g));                   // run + reply
        return;                                                    // done
    }

    if (lower(mode) == "manual") {                                // MANUAL branch
        Graph g; std::string err;                                 // output + error
        if (!parse_manual_line(line, g, err)) {                   // parse
            send_all(cfd, "Error: " + err + "\n");                // send error
            return;                                               // bail
        }
        send_all(cfd, run_and_format(name, g));                   // run + reply
        return;                                                    // done
    }

    send_all(cfd, "Bad mode. Use RANDOM or MANUAL.\n");           // unknown mode
}

// ---------- accept a client ----------
static void accept_client(int sfd) {
    sockaddr_storage a{}; socklen_t alen = sizeof(a);             // peer addr storage
    int cfd = ::accept(sfd, (sockaddr*)&a, &alen);                // accept()
    if (cfd < 0) { perror("accept"); return; }                    // guard
    g_fds.push_back({cfd, POLLIN, 0});                            // watch for reads
    std::cout << "[server] client fd=" << cfd << " connected\n";  // log
}

// ---------- read one request from a client and process ----------
static void read_once(std::size_t idx) {
    auto& p = g_fds[idx];                                         // pollfd ref
    char buf[kBufSize];                                           // recv buffer
    ssize_t n = ::recv(p.fd, buf, sizeof(buf) - 1, 0);            // receive bytes
    if (n <= 0) {                                                 // disconnect or error
        std::cout << "[server] client " << p.fd << " disconnected\n"; // log
        ::close(p.fd); p.fd = -1; p.events = 0; p.revents = 0;    // mark as closed
        return;                                                   // done
    }
    buf[n] = '\0';                                                // null-terminate buffer
    std::string line(buf);                                        // convert to std::string
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) // trim CR/LF
        line.pop_back();                                          // drop trailing newline
    std::cout << "[server] fd=" << p.fd << " cmd: " << line << "\n"; // log
    handle_command(p.fd, line);                                   // parse + execute
}

int main() {
    std::signal(SIGINT, on_sigint);                               // install Ctrl+C handler

    addrinfo hints{};                                             // zero-init hints
    hints.ai_family   = AF_INET;                                   // IPv4
    hints.ai_socktype = SOCK_STREAM;                               // TCP
    hints.ai_flags    = AI_PASSIVE;                                // we will bind

    addrinfo* res = nullptr;                                       // result list
    if (getaddrinfo(kIP, kPort, &hints, &res) != 0) {              // resolve bind address
        perror("getaddrinfo"); return 1;                           // fail on error
    }

    int sfd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol); // create socket
    if (sfd < 0) { perror("socket"); freeaddrinfo(res); return 1; }         // guard

    int yes = 1;                                                    // opt value
    ::setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); // quick-rebind

    if (::bind(sfd, res->ai_addr, res->ai_addrlen) < 0) {           // bind to ip:port
        perror("bind"); ::close(sfd); freeaddrinfo(res); return 1;  // cleanup on error
    }
    if (::listen(sfd, kBacklog) < 0) {                              // start listening
        perror("listen"); ::close(sfd); freeaddrinfo(res); return 1;// cleanup on error
    }
    freeaddrinfo(res);                                              // free addr list

    g_fds.push_back({sfd, POLLIN, 0});                              // watch the listener
    std::cout << "[server] listening on " << kIP << ":" << kPort << "\n"; // banner

    while (true) {                                                  // event loop
        int nready = ::poll(g_fds.data(), g_fds.size(), kNoTimeout);// wait for events
        if (nready < 0) {                                           // poll error
            if (errno == EINTR) continue;                           // interrupted → resume
            perror("poll"); break;                                  // otherwise bail
        }
        for (std::size_t i = 0; i < g_fds.size() && nready > 0; ++i) { // scan fds
            auto& p = g_fds[i];                                     // ref
            if (!(p.revents & POLLIN)) continue;                    // only handle readable
            --nready;                                               // one event handled
            if (p.fd == sfd) accept_client(sfd);                    // new connection
            else              read_once(i);                         // client data
        }
        g_fds.erase(std::remove_if(g_fds.begin(), g_fds.end(),      // compact vector
                    [](const pollfd& x){ return x.fd == -1; }),
                    g_fds.end());
    }

    close_all();                                                    // close sockets
    return 0;                                                       // done
}
