// ==========================
// Part 3: Random graph + Euler (undirected or directed)
// ==========================
// Parses: -v <V> -e <E> -s <seed> [--directed]
// Generates a random graph and runs Euler on it (both modes supported).
// ==========================

#include "graph/Graph.hpp"    // Graph API
#include "algo/Euler.hpp"     // Euler algorithm
#include <getopt.h>           // getopt_long for command-line parsing
#include <cstdlib>            // std::atoi, std::exit
#include <iostream>           // I/O
#include <random>             // PRNG
#include <set>                // deduplicate edges

static void usage(const char* prog) {                         // print usage and exit
    std::cerr << "Usage: " << prog
              << " -v <vertices> -e <edges> -s <seed> [--directed]\n";
    std::exit(1);
}

int main(int argc, char* argv[]) {                            // entry point
    long long V=-1, E=-1, SEED=-1; bool dir=false; int li=0;  // defaults and parsing state
    option lo[] = {{"directed", no_argument, nullptr, 'D'}, {nullptr,0,nullptr,0}}; // long options

    for (int opt; (opt = getopt_long(argc, argv, "v:e:s:", lo, &li)) != -1; ) { // parse flags
        if (opt=='v') V = std::atoi(optarg);                  // vertices
        else if (opt=='e') E = std::atoi(optarg);             // edges
        else if (opt=='s') SEED = std::atoi(optarg);          // seed
        else if (opt=='D') dir = true;                        // directed flag
        else usage(argv[0]);                                   // invalid flag
    }

    if (V<=0 || E<0 || SEED<0) usage(argv[0]);                // basic validation
    if (!dir && E > V*(V-1)/2) {                              // max edges in simple undirected graph
        std::cerr << "Too many edges for a simple undirected graph\n";
        return 1;
    }

    Graph g( static_cast<std::size_t>(V),
             dir ? Graph::Kind::Directed : Graph::Kind::Undirected,
             {false,false});                                  // no self-loops, no multi-edges

    std::mt19937 rng(static_cast<std::uint32_t>(SEED));       // PRNG
    std::uniform_int_distribution<int> pick(0, (int)V-1);     // uniform vertex picker

    std::set<std::pair<int,int>> used;                        // avoid duplicates
    long long added = 0;                                      // edges added

    while (added < E) {                                       // keep adding edges
        int u = pick(rng), v = pick(rng);                     // random endpoints
        if (u == v) continue;                                 // skip self-loops
        if (!dir) {                                           // undirected key (ordered)
            auto k = std::minmax(u,v);
            if (used.count(k)) continue;
            g.addEdge(u,v,1);                                 // add undirected edge (Graph adds both arcs)
            used.insert(k);
        } else {                                              // directed key (ordered u->v)
            auto k = std::make_pair(u,v);
            if (used.count(k)) continue;
            g.addEdge(u,v,1);                                 // add directed arc with capacity 1
            used.insert(k);
        }
        ++added;                                              // count it
    }

    std::cout << "Generated " << g.label() << "\n";           // summary

    Euler solver;                                             // algorithm object
    std::cout << solver.run(g) << "\n";                       // run Euler (works for both modes)

    return 0;                                                 // success
}
