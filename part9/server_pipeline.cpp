// ======================= server_pipeline.cpp (Part 9) =======================  // file header explaining purpose
// Multithreaded TCP server using the **Pipeline** pattern + **Active Objects**. // high-level design summary
//                                                                              // spacer
// Request/Response protocol (one newline-terminated line per client):          // protocol intro
//   ALG ALL RANDOM <V> <E> <SEED> [--directed]                                 // random-mode syntax
//   ALG ALL MANUAL <V> : u-v u-v ... [--directed]                              // manual-mode syntax
//                                                                              // spacer
// Stages (each is an Active Object = a thread + a blocking queue):             // pipeline overview
//   [MAIN acceptor] -> (Stage 1) Parse+BuildGraph AO                            // accept -> parser stage
//                    -> (Stage 2) Dispatcher AO (fan out)                      // then to dispatcher
//                    -> (Stage 3a) MST AO                                      // dedicated algorithm workers
//                    -> (Stage 3b) SCC AO                                       // ...
//                    -> (Stage 3c) MAXFLOW AO                                   // ...
//                    -> (Stage 3d) HAMILTON AO                                  // ...
//                    -> (Stage 4) Aggregator AO (fan in)                        // results merged
//                    -> (Stage 5) Sender AO                                     // send reply
//                                                                              // spacer
// Notes:                                                                       // notes section
//  * Reuses your Part-7 Strategy/Factory via AlgorithmFactory.                 // reuse of existing code
//  * Uses a simple thread-safe BlockingQueue<T> per stage.                     // mailbox per stage
//  * Clean shutdown on Ctrl+C.                                                 // shutdown behavior
// ============================================================================ // end banner

#include "graph/Graph.hpp"             // our Graph API (in include/graph)                       // include graph interface
#include "algo/GraphAlgorithm.hpp"     // IGraphAlgorithm + AlgorithmFactory                      // include strategy/factory

#include <arpa/inet.h>                 // inet_pton, htons, etc.                                 // sockets header
#include <netdb.h>                     // getaddrinfo, freeaddrinfo                              // address resolution
#include <sys/socket.h>                // socket, bind, listen, accept, send, recv               // socket API
#include <unistd.h>                    // close, shutdown                                        // POSIX close/shutdown

#include <algorithm>                   // std::minmax                                            // algorithm utilities
#include <atomic>                      // std::atomic                                            // atomic flags
#include <condition_variable>          // std::condition_variable                                // threading primitive
#include <csignal>                     // std::signal                                            // signal handling
#include <cstring>                     // std::strerror                                          // C string utilities
#include <iostream>                    // std::cout, std::cerr                                   // IO streams
#include <map>                         // std::map                                               // map for aggregator
#include <memory>                      // std::shared_ptr, std::make_shared                      // smart pointers
#include <mutex>                       // std::mutex, std::lock_guard, std::unique_lock          // mutex types
#include <optional>                    // std::optional                                          // optional return
#include <queue>                       // std::queue                                             // queue container
#include <random>                      // std::mt19937, distributions                            // RNG for random graphs
#include <set>                         // std::set                                               // dedup edges
#include <sstream>                     // std::istringstream, std::ostringstream                 // string streams
#include <string>                      // std::string                                            // strings
#include <thread>                      // std::thread                                            // threads
#include <utility>                     // std::move, std::pair                                   // utility
#include <vector>                      // std::vector                                            // vectors

// ============ basic config ============
static constexpr const char* kIP   = "127.0.0.1";           // bind IP (loopback)
static constexpr const char* kPort = "5555";                // bind port (string)
static constexpr int         kBacklog = 32;                 // listen backlog
static constexpr int         kBufSz   = 4096;               // recv buffer size

// ============ small helpers ============
static std::string lower(std::string s) {                   // lowercase helper
    for (auto& c : s) c = (char)std::tolower((unsigned char)c); // tolower safely
    return s;                                               // return modified string
}

static void send_all(int fd, const std::string& s) {        // send whole string
    const char* p = s.c_str();                              // raw pointer to data
    std::size_t left = s.size();                            // bytes left to send
    while (left) {                                          // loop until done
        ssize_t n = ::send(fd, p, left, 0);                 // attempt to send
        if (n <= 0) return;                                 // bail on error/closed
        p += n;                                             // advance pointer
        left -= (std::size_t)n;                             // reduce remaining
    }
}

// ============ BlockingQueue<T> (Active Object mailbox) ============
template <typename T>
class BlockingQueue {                                       // simple thread-safe queue
public:
    void push(T v) {                                        // enqueue an item
        {
            std::lock_guard<std::mutex> lk(mu_);            // lock mutex
            if (closed_) return;                            // ignore if closed
            q_.push(std::move(v));                          // push item
        }
        cv_.notify_one();                                   // wake a waiting pop
    }

    // Blocks until item available or queue closed; returns nullopt when closed and empty.
    std::optional<T> pop() {                                // dequeue (blocking)
        std::unique_lock<std::mutex> lk(mu_);              // lock with unique_lock
        cv_.wait(lk, [&]{ return closed_ || !q_.empty(); });// wait for item/close
        if (q_.empty()) return std::nullopt;                // closed and drained
        T v = std::move(q_.front());                        // take front
        q_.pop();                                           // pop it
        return v;                                           // return item
    }

    void close() {                                          // close the queue
        {
            std::lock_guard<std::mutex> lk(mu_);            // lock
            closed_ = true;                                 // mark closed
        }
        cv_.notify_all();                                   // wake all waiters
    }

private:
    std::mutex mu_;                                         // protects queue
    std::condition_variable cv_;                            // signals availability
    std::queue<T> q_;                                       // FIFO storage
    bool closed_ = false;                                   // closed flag
};

// ============ job types carried through the pipeline ============
using ReqId = uint64_t;                                     // request identifier type

struct ClientMsg {                                          // message from acceptor to parser
    int         client_fd;                                  // client socket fd
    std::string line;                                       // input line
    ReqId       id;                                         // request id
};

struct GraphJob {                                           // parsed/built graph for dispatch
    int                          client_fd;                 // client socket fd
    std::shared_ptr<Graph>       g;                         // shared graph
    std::string                  label;                     // graph label text
    ReqId                        id;                        // request id
};

struct AlgoTask {                                           // task for a specific algorithm
    int                          client_fd;                 // client socket fd
    std::shared_ptr<Graph>       g;                         // shared graph
    std::string                  algoName;                  // "MST","SCC","MAXFLOW","HAMILTON"
    std::string                  label;                     // graph label text
    ReqId                        id;                        // request id
};

struct AlgoResult {                                         // result produced by an algorithm worker
    int         client_fd;                                   // client socket fd
    std::string algoName;                                    // algorithm name
    std::string text;                                        // human-readable result
    std::string label;                                       // graph label (for header)
    ReqId       id;                                          // request id
};

struct Response {                                           // final aggregated response
    int         client_fd;                                   // client socket fd
    std::string payload;                                     // payload to send
    ReqId       id;                                          // request id (unused in sender)
};

// ============ global stop flag + sigint ============
static std::atomic<bool> g_stop{false};                     // global stop flag
static int g_listen_fd = -1;                                // listening socket

static void close_listen_fd() {                             // close listening socket
    if (g_listen_fd >= 0) { ::close(g_listen_fd); g_listen_fd = -1; } // close if open
}

static void on_sigint(int) {                                // SIGINT handler
    g_stop.store(true);                                     // set stop flag
    close_listen_fd();                                      // unblock accept()
}

// ============ Graph builders (same semantics as part 8) ============
static Graph make_random_graph(std::size_t V, std::size_t E, unsigned seed, bool directed) { // build random graph
    Graph::Options opt; opt.allowSelfLoops=false; opt.allowMultiEdges=false; // disallow loops/multiedges
    Graph g(V, directed? Graph::Kind::Directed : Graph::Kind::Undirected, opt); // construct graph

    std::mt19937 rng(seed);                                 // PRNG
    std::uniform_int_distribution<int> pick(0,(int)V-1);    // vertex picker
    std::set<std::pair<int,int>> seen;                      // dedupe set
    std::size_t added = 0;                                  // edges added

    while (!g_stop.load() && added < E) {                   // until enough edges or stop
        int u = pick(rng), v = pick(rng);                   // choose endpoints
        if (u == v) continue;                               // skip self-loop
        if (directed) {                                     // directed case
            auto key = std::make_pair(u,v);                 // directed unique key
            if (seen.count(key)) continue;                  // skip duplicate arc
            g.addEdge(u,v,1);                               // add arc with weight 1
            seen.insert(key);                               // remember arc
        } else {                                            // undirected case
            auto mm = std::minmax(u,v);                     // canonical (min,max)
            if (seen.count(mm)) continue;                   // skip duplicate edge
            g.addEdge(u,v,1);                               // add undirected edge
            seen.insert(mm);                                // remember edge
        }
        ++added;                                            // count edge
    }
    return g;                                               // return graph
}

// Parse: "ALG ALL MANUAL <V> : u-v u-v ... [--directed]"
static bool parse_manual_all(const std::string& line, Graph& out, std::string& err) { // parse manual command
    std::istringstream iss(line);                         // tokenizer
    std::string kw1, kw2, mode; iss >> kw1 >> kw2 >> mode; // read ALG ALL MODE
    if (lower(kw1)!="alg" || lower(kw2)!="all" || lower(mode)!="manual") { // validate
        err = "Expected: ALG ALL MANUAL <V> : u-v u-v ... [--directed]";    // error text
        return false;                                                       // fail
    }
    std::size_t V=0; char colon=0; iss >> V >> colon;       // read V and ':'
    if (V==0 || colon!=':') { err = "Format: ALG ALL MANUAL <V> : u-v ... [--directed]"; return false; } // validate

    std::vector<std::string> toks; std::string tok;         // tokens after ':'
    while (iss >> tok) toks.push_back(tok);                 // collect tokens

    bool directed=false;                                    // directed flag
    if (!toks.empty() && toks.back()=="--directed"){ directed=true; toks.pop_back(); } // trailing flag

    Graph::Options opt; opt.allowSelfLoops=false; opt.allowMultiEdges=false; // options
    out = Graph(V, directed? Graph::Kind::Directed : Graph::Kind::Undirected, opt); // build target graph

    std::set<std::pair<int,int>> seen;                      // dedupe set
    for (auto& t: toks) {                                   // for each edge token
        auto dash = t.find('-');                            // locate '-'
        if (dash==std::string::npos) { err="Bad token: "+t; return false; } // malformed
        int u = std::stoi(t.substr(0,dash));                // parse u
        int v = std::stoi(t.substr(dash+1));                // parse v
        if (u<0 || v<0 || (std::size_t)u>=V || (std::size_t)v>=V || u==v) { err="Invalid endpoints: "+t; return false; } // validate range
        if (out.directed()) {                                // directed
            auto key = std::make_pair(u,v);                 // directed key
            if (seen.count(key)) { err="Duplicate arc: "+t; return false; } // check dup
            seen.insert(key); out.addEdge(u,v,1);           // add arc
        } else {                                            // undirected
            auto mm = std::minmax(u,v);                     // canon pair
            if (seen.count(mm)) { err="Duplicate edge: "+t; return false; } // check dup
            seen.insert(mm); out.addEdge(u,v,1);            // add edge
        }
    }
    return true;                                            // success
}

static bool build_graph_from_command(const std::string& line, Graph& out, std::string& err) { // parse RANDOM/MANUAL
    std::istringstream iss(line);                           // tokenizer
    std::string kw1, kw2, mode; iss >> kw1 >> kw2 >> mode;  // read ALG ALL MODE

    if (lower(kw1)!="alg" || lower(kw2)!="all") {           // validate prefix
        err = "Unknown. Use:\n"                             // guidance text
              "  ALG ALL RANDOM <V> <E> <SEED> [--directed]\n"
              "  ALG ALL MANUAL <V> : u-v u-v ... [--directed]\n";
        return false;                                       // fail
    }

    if (lower(mode) == "random") {                          // RANDOM path
        std::size_t V=0, E=0; unsigned seed=0; std::string flag; // params
        iss >> V >> E >> seed >> flag;                      // parse params
        if (V==0) { err="V must be > 0"; return false; }    // validate V
        const bool directed = (flag=="--directed");         // detect flag
        out = make_random_graph(V, E, seed, directed);      // build random graph
        return true;                                        // success
    }
    if (lower(mode) == "manual") return parse_manual_all(line, out, err); // delegate manual

    err = "Bad mode. Use RANDOM or MANUAL.";                // unknown mode
    return false;                                           // fail
}

// ============ Active Objects (stages) ============

// -------- Stage 1: Parse + Build Graph --------
class ParserStage {                                         // parser AO
public:
    ParserStage(BlockingQueue<ClientMsg>& in, BlockingQueue<GraphJob>& out) // ctor wires queues
      : in_(in), out_(out), th_([this]{ run(); }) {}        // spawn thread running run()

    void join() { if (th_.joinable()) th_.join(); }         // join on shutdown

private:
    void run() {                                            // thread body
        while (!g_stop.load()) {                            // loop until stop
            auto msg = in_.pop();                           // pop a client message
            if (!msg) break;                                // queue drained
            Graph g; std::string err;                       // local graph + error
            if (!build_graph_from_command(msg->line, g, err)) { // parse/build
                send_all(msg->client_fd, "Error: " + err + "\n"); // send error
                ::shutdown(msg->client_fd, SHUT_RDWR);      // shutdown socket
                ::close(msg->client_fd);                    // close socket
                continue;                                   // next item
            }
            auto sp = std::make_shared<Graph>(std::move(g)); // share graph
            GraphJob gj{ msg->client_fd, sp, sp->label(), msg->id }; // build job
            out_.push(std::move(gj));                       // push to next stage
        }
    }

    BlockingQueue<ClientMsg>& in_;                          // input queue
    BlockingQueue<GraphJob>&  out_;                         // output queue
    std::thread th_;                                        // worker thread
};

// -------- Stage 2: Dispatcher (fan-out to 4 algorithm queues) --------
class DispatcherStage {                                     // dispatcher AO
public:
    DispatcherStage(BlockingQueue<GraphJob>& in,            // ctor with all queues
                    BlockingQueue<AlgoTask>& q_mst,
                    BlockingQueue<AlgoTask>& q_scc,
                    BlockingQueue<AlgoTask>& q_maxflow,
                    BlockingQueue<AlgoTask>& q_hamilton,
                    BlockingQueue<AlgoResult>& q_agg_start)
      : in_(in), q_mst_(q_mst), q_scc_(q_scc), q_max_(q_maxflow), q_ham_(q_hamilton),
        q_agg_(q_agg_start), th_([this]{ run(); }) {}       // start thread

    void join() { if (th_.joinable()) th_.join(); }         // join on shutdown

private:
    void run() {                                            // thread body
        while (!g_stop.load()) {                            // loop
            auto gj = in_.pop();                            // pop a graph job
            if (!gj) break;                                 // drained
            // Tell aggregator a new request is coming (so it knows the label & expects 4 results).
            q_agg_.push(AlgoResult{ gj->client_fd, "BEGIN", "", gj->label, gj->id }); // BEGIN sentinel

            // fan out to the four algorithm workers
            q_mst_.push(     AlgoTask{ gj->client_fd, gj->g, "MST",      gj->label, gj->id }); // MST task
            q_scc_.push(     AlgoTask{ gj->client_fd, gj->g, "SCC",      gj->label, gj->id }); // SCC task
            q_max_.push(     AlgoTask{ gj->client_fd, gj->g, "MAXFLOW",  gj->label, gj->id }); // MaxFlow task
            q_ham_.push(     AlgoTask{ gj->client_fd, gj->g, "HAMILTON", gj->label, gj->id }); // Hamilton task
        }
    }

    BlockingQueue<GraphJob>& in_;                           // input queue
    BlockingQueue<AlgoTask>& q_mst_;                        // MST queue
    BlockingQueue<AlgoTask>& q_scc_;                        // SCC queue
    BlockingQueue<AlgoTask>& q_max_;                        // MAXFLOW queue
    BlockingQueue<AlgoTask>& q_ham_;                        // HAMILTON queue
    BlockingQueue<AlgoResult>& q_agg_;                      // aggregator input
    std::thread th_;                                        // worker thread
};

// -------- Stage 3: Algorithm worker (used 4 times) --------
class AlgoWorker {                                          // algorithm AO
public:
    AlgoWorker(const char* name, BlockingQueue<AlgoTask>& in, BlockingQueue<AlgoResult>& out) // ctor
      : name_(name), in_(in), out_(out), th_([this]{ run(); }) {} // spawn thread

    void join() { if (th_.joinable()) th_.join(); }         // join on shutdown

private:
    void run() {                                            // thread body
        while (!g_stop.load()) {                            // loop
            auto t = in_.pop();                             // pop a task
            if (!t) break;                                  // drained
            auto alg = AlgorithmFactory::create(t->algoName); // create strategy
            std::string text = alg ? alg->run(*t->g) : "(unavailable)"; // run or fallback
            out_.push(AlgoResult{ t->client_fd, t->algoName, text, t->label, t->id }); // push result
        }
    }

    std::string name_;                                      // worker name (unused for logic)
    BlockingQueue<AlgoTask>& in_;                           // input queue
    BlockingQueue<AlgoResult>& out_;                        // output queue
    std::thread th_;                                        // worker thread
};

// -------- Stage 4: Aggregator (fan-in) --------
class AggregatorStage {                                     // aggregator AO
public:
    AggregatorStage(BlockingQueue<AlgoResult>& in, BlockingQueue<Response>& out) // ctor
      : in_(in), out_(out), th_([this]{ run(); }) {}        // spawn thread

    void join() { if (th_.joinable()) th_.join(); }         // join on shutdown

private:
    struct State {                                          // per-request state
        int client_fd = -1;                                 // client fd
        std::string label;                                  // graph label
        std::map<std::string,std::string> got;              // algoName -> result
    };

    void run() {                                            // thread body
        while (!g_stop.load()) {                            // loop
            auto r = in_.pop();                             // pop a result
            if (!r) break;                                  // drained

            auto& st = reqs_[r->id];                        // get/create state
            if (r->algoName == "BEGIN") {                   // BEGIN sentinel?
                st.client_fd = r->client_fd;                // record fd
                st.label     = r->label;                    // record label
                continue;                                   // wait for results
            }

            st.client_fd = r->client_fd;                    // update fd
            st.label     = r->label;                        // update label
            st.got[r->algoName] = r->text;                  // record this result

            if (st.got.size() == 4) {                       // all four arrived?
                std::ostringstream oss;                     // build response
                oss << "Graph: " << st.label << "\n";       // header
                oss << "MST: "      << st.got["MST"]      << "\n"; // MST line
                oss << "SCC: "      << st.got["SCC"]      << "\n"; // SCC line
                oss << "MAXFLOW: "  << st.got["MAXFLOW"]  << "\n"; // MaxFlow line
                oss << "HAMILTON: " << st.got["HAMILTON"] << "\n"; // Hamilton line
                out_.push(Response{ st.client_fd, oss.str(), /*id*/0 }); // enqueue response
                reqs_.erase(r->id);                         // discard state
            }
        }
    }

    BlockingQueue<AlgoResult>& in_;                         // input queue
    BlockingQueue<Response>&   out_;                        // output queue
    std::map<ReqId, State>     reqs_;                       // per-request map
    std::thread th_;                                        // worker thread
};

// -------- Stage 5: Sender --------
class SenderStage {                                         // sender AO
public:
    SenderStage(BlockingQueue<Response>& in) : in_(in), th_([this]{ run(); }) {} // ctor + start
    void join() { if (th_.joinable()) th_.join(); }         // join on shutdown

private:
    void run() {                                            // thread body
        while (!g_stop.load()) {                            // loop
            auto r = in_.pop();                             // pop a response
            if (!r) break;                                  // drained
            send_all(r->client_fd, r->payload);             // send payload
            ::shutdown(r->client_fd, SHUT_RDWR);            // shutdown socket
            ::close(r->client_fd);                          // close socket
        }
    }

    BlockingQueue<Response>& in_;                           // input queue
    std::thread th_;                                        // worker thread
};

// ============ network setup ============
static bool setup_listen() {                                // create/bind/listen
    addrinfo hints{}; hints.ai_family=AF_INET; hints.ai_socktype=SOCK_STREAM; hints.ai_flags=AI_PASSIVE; // hints
    addrinfo* res=nullptr;                                  // result pointer
    if (getaddrinfo(kIP,kPort,&hints,&res)!=0) { std::perror("getaddrinfo"); return false; } // resolve
    g_listen_fd = ::socket(res->ai_family,res->ai_socktype,res->ai_protocol); // create socket
    if (g_listen_fd<0) { std::perror("socket"); freeaddrinfo(res); return false; } // check
    int yes=1; ::setsockopt(g_listen_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)); // reuse addr
    if (::bind(g_listen_fd,res->ai_addr,res->ai_addrlen)<0) { std::perror("bind"); freeaddrinfo(res); close_listen_fd(); return false; } // bind
    if (::listen(g_listen_fd,kBacklog)<0) { std::perror("listen"); freeaddrinfo(res); close_listen_fd(); return false; } // listen
    freeaddrinfo(res);                                      // free addrinfo
    return true;                                            // success
}

// ============ main ============
int main() {                                                // program entry
    std::signal(SIGINT, on_sigint);                         // install SIGINT handler

    if (!setup_listen()) return 1;                          // setup server socket
    std::cout << "[Pipeline server] listening on " << kIP << ":" << kPort << "\n"; // log

    // Mailboxes
    BlockingQueue<ClientMsg>   q_in;                        // acceptor -> parser
    BlockingQueue<GraphJob>    q_graph;                     // parser -> dispatcher
    BlockingQueue<AlgoTask>    q_mst, q_scc, q_max, q_ham;  // dispatcher -> workers
    BlockingQueue<AlgoResult>  q_agg_in;                    // workers -> aggregator
    BlockingQueue<Response>    q_send;                      // aggregator -> sender

    // Stages
    ParserStage     stage_parse(q_in, q_graph);             // start parser AO
    DispatcherStage stage_disp(q_graph, q_mst, q_scc, q_max, q_ham, q_agg_in); // start dispatcher AO
    AlgoWorker      w_mst ("MST",      q_mst, q_agg_in);    // start MST AO
    AlgoWorker      w_scc ("SCC",      q_scc, q_agg_in);    // start SCC AO
    AlgoWorker      w_max ("MAXFLOW",  q_max, q_agg_in);    // start MAXFLOW AO
    AlgoWorker      w_ham ("HAMILTON", q_ham, q_agg_in);    // start HAMILTON AO
    AggregatorStage stage_agg(q_agg_in, q_send);            // start aggregator AO
    SenderStage     stage_send(q_send);                     // start sender AO

    // Simple accept loop: read one command line per connection, enqueue to pipeline.
    ReqId next_id = 1;                                      // monotonic request id
    while (!g_stop.load()) {                                // accept loop
        sockaddr_storage a{}; socklen_t alen=sizeof(a);     // client address
        int cfd = ::accept(g_listen_fd,(sockaddr*)&a,&alen);// accept client
        if (cfd < 0) {                                      // if failed
            if (g_stop.load()) break;                       // exit on stop
            // transient error; continue
            continue;                                       // try again
        }
        // read one line
        char buf[kBufSz];                                   // recv buffer
        ssize_t n = ::recv(cfd, buf, sizeof(buf)-1, 0);     // receive data
        if (n <= 0) { ::close(cfd); continue; }             // close on error/EOF
        buf[n] = '\0';                                      // C-string terminate

        std::string line(buf);                              // wrap as std::string
        // Trim CR/LF
        while (!line.empty() && (line.back()=='\n' || line.back()=='\r')) line.pop_back(); // strip CR/LF

        q_in.push(ClientMsg{ cfd, std::move(line), next_id++ }); // enqueue to parser
    }

    // Shutdown: close listening socket and drain queues
    close_listen_fd();                                      // close listener
    q_in.close();                                           // close queues in order
    q_graph.close();                                        // ...
    q_mst.close(); q_scc.close(); q_max.close(); q_ham.close(); // close worker queues
    q_agg_in.close();                                       // close aggregator in
    q_send.close();                                         // close sender in

    // Join stages
    stage_parse.join();                                     // join parser
    stage_disp.join();                                      // join dispatcher
    w_mst.join(); w_scc.join(); w_max.join(); w_ham.join(); // join workers
    stage_agg.join();                                       // join aggregator
    stage_send.join();                                      // join sender

    return 0;                                               // done
}
