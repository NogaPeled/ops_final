#include "algo/Euler.hpp"                 // include our header so the compiler sees the class
#include <vector>                         // vectors for visited flags, adjacency copies, and the circuit
#include <stack>                          // stack used by Hierholzer's algorithm
#include <sstream>                        // ostringstream to format the output string

// -----------------------------
// Helper (undirected): DFS for reachability among non-isolated vertices
// -----------------------------
static void dfs_undirected(Graph::Vertex u, const Graph& g, std::vector<bool>& seen) {
    seen[u] = true;                                               // mark current as seen
    for (const auto& e : g.adj(u)) {                              // scan neighbors
        Graph::Vertex v = e.first;                                // neighbor id
        if (!seen[v]) dfs_undirected(v, g, seen);                 // recurse if not visited
    }
}

// -----------------------------
// Helper (directed): DFS following arc directions
// -----------------------------
static void dfs_directed(Graph::Vertex u, const Graph& g, std::vector<bool>& seen) {
    seen[u] = true;                                               // mark current
    for (const auto& e : g.adj(u)) {                              // iterate outgoing arcs u->v
        Graph::Vertex v = e.first;                                // target vertex
        if (!seen[v]) dfs_directed(v, g, seen);                   // recurse along direction
    }
}

// -----------------------------
// Build Euler circuit for UNDIRECTED graphs using Hierholzer with edge IDs
// -----------------------------
static std::string euler_undirected(const Graph& g) {
    const std::size_t n = g.n();                                  // number of vertices

    // 1) degree must be even for all vertices + pick a start with deg>0
    int odd = 0;                                                  // count odd-degree vertices
    Graph::Vertex start = n;                                      // start vertex among non-isolated
    for (Graph::Vertex u = 0; u < n; ++u) {                       // check each vertex
        std::size_t deg = g.adj(u).size();                        // undirected degree = list size
        if (deg % 2) ++odd;                                       // count odds
        if (deg > 0 && start == n) start = u;                     // remember a start point
    }
    if (odd > 0) return "No Euler circuit: at least one vertex has odd degree."; // fail on parity

    if (start == n) return "Graph has no edges; trivial Euler circuit at vertex 0."; // empty-edges case

    // 2) connectivity among non-isolated vertices
    std::vector<bool> seen(n, false);                             // visited flags
    dfs_undirected(start, g, seen);                                // reachability
    for (Graph::Vertex u = 0; u < n; ++u) {                       // verify every non-isolated is seen
        if (!seen[u] && !g.adj(u).empty())                        // unreachable but has edges
            return "No Euler circuit: graph is disconnected on non-isolated vertices.";
    }

    // 3) Hierholzer using undirected-edge IDs so each is used once
    struct EdgeRef { Graph::Vertex to; int id; };                 // entry in working adjacency
    std::vector<std::vector<EdgeRef>> adj(n);                     // working adjacency
    int eid = 0;                                                  // unique undirected edge id

    // create paired entries with the same id (one per undirected edge)
    for (Graph::Vertex u = 0; u < n; ++u) {                       // for each vertex
        for (const auto& p : g.adj(u)) {                          // for each neighbor
            Graph::Vertex v = p.first;                            // neighbor index
            if (u <= v) {                                         // ensure we assign id once
                adj[u].push_back({v, eid});                       // u->v with id
                if (u != v) adj[v].push_back({u, eid});           // v->u with same id (avoid self-loop dup)
                ++eid;                                            // next id
            }
        }
    }

    std::vector<bool> used(eid, false);                           // used flags per undirected edge id
    std::stack<Graph::Vertex> st;                                 // traversal stack
    std::vector<Graph::Vertex> circuit;                           // resulting circuit vertices
    st.push(start);                                               // begin from start

    while (!st.empty()) {                                         // while there is path to explore
        Graph::Vertex u = st.top();                               // current vertex
        // drop used edges at the tail of list
        while (!adj[u].empty() && used[adj[u].back().id]) adj[u].pop_back(); // prune
        if (!adj[u].empty()) {                                    // have an unused edge
            EdgeRef e = adj[u].back();                            // take it
            used[e.id] = true;                                    // mark id used
            st.push(e.to);                                        // move to neighbor
        } else {                                                  // dead end, backtrack
            circuit.push_back(u);                                 // record vertex in circuit
            st.pop();                                             // pop stack
        }
    }

    if (circuit.size() != static_cast<std::size_t>(eid) + 1) {    // sanity: must visit all edges
        return "No Euler circuit: not all edges were traversed (sanity check failed).";
    }

    // format output
    std::ostringstream oss;                                       // build string
    oss << "Euler circuit: ";                                     
    for (std::size_t i = 0; i < circuit.size(); ++i) {
        oss << circuit[i];
        if (i + 1 < circuit.size()) oss << " -> ";
    }
    return oss.str();                                             // return final message
}

// -----------------------------
// Build Euler circuit for DIRECTED graphs using Hierholzer on arcs
// Conditions:
//   1) For every vertex: in-degree == out-degree
//   2) All vertices with degree>0 are strongly connected (both ways)
// -----------------------------
static std::string euler_directed(const Graph& g) {
    const std::size_t n = g.n();                                  // number of vertices

    // 1) In-degree equals out-degree for every vertex
    std::vector<std::size_t> out = g.outDegree();                 // out-degrees
    std::vector<std::size_t> in  = g.inDegree();                  // in-degrees
    Graph::Vertex start = n;                                      // start at a vertex with out>0
    for (Graph::Vertex u = 0; u < n; ++u) {                       
        if (in[u] != out[u])                                      // mismatch violates the condition
            return "No Euler circuit (directed): in-degree != out-degree at some vertex.";
        if (out[u] > 0 && start == n) start = u;                  // remember a start vertex with edges
    }
    if (start == n) return "Graph has no edges; trivial Euler circuit at vertex 0."; // empty-edges case

    // 2) Strong connectivity among vertices with degree>0
    //    We check reachability in both directions: G and G^R starting from 'start'
    std::vector<bool> seenF(n, false);                            // seen in forward graph
    dfs_directed(start, g, seenF);                                 // DFS following arcs
    Graph gr = g.reversed();                                      // reversed graph
    std::vector<bool> seenR(n, false);                            // seen in reverse graph
    dfs_directed(start, gr, seenR);                                // DFS in reversed arcs

    for (Graph::Vertex u = 0; u < n; ++u) {                       // for each vertex
        if ((in[u] + out[u]) == 0) continue;                      // ignore isolated
        if (!seenF[u] || !seenR[u])                               // must be reachable both ways
            return "No Euler circuit (directed): graph is not strongly connected on non-isolated vertices.";
    }

    // 3) Hierholzer on directed arcs: just consume arcs once
    std::vector<std::vector<Graph::Vertex>> adj(n);               // working adjacency: list of neighbors
    for (Graph::Vertex u = 0; u < n; ++u) {                       // copy adjacency
        adj[u].reserve(g.adj(u).size());                          // reserve capacity
        for (const auto& e : g.adj(u)) adj[u].push_back(e.first); // store only targets (each arc once)
    }

    std::stack<Graph::Vertex> st;                                 // traversal stack
    std::vector<Graph::Vertex> circuit;                           // resulting circuit
    st.push(start);                                               // begin

    while (!st.empty()) {                                         // while there is path to follow
        Graph::Vertex u = st.top();                               // current vertex
        if (!adj[u].empty()) {                                    // have an unused outgoing arc
            Graph::Vertex v = adj[u].back();                      // take last outgoing arc
            adj[u].pop_back();                                    // consume this arc
            st.push(v);                                           // move to neighbor
        } else {                                                  // dead end: record and backtrack
            circuit.push_back(u);                                 // add vertex to path
            st.pop();                                             // pop stack
        }
    }

    // Directed circuit length must be (#arcs) + 1; #arcs == sum(out) == sum(in)
    std::size_t arcs = 0;                                         // count arcs
    for (auto x : out) arcs += x;                                 // sum of out-degrees equals number of arcs
    if (circuit.size() != arcs + 1) {                             // sanity check for full traversal
        return "No Euler circuit (directed): not all arcs were traversed (sanity check failed).";
    }

    // format output
    std::ostringstream oss;                                       // build readable result
    oss << "Euler circuit (directed): ";
    for (std::size_t i = 0; i < circuit.size(); ++i) {
        oss << circuit[i];
        if (i + 1 < circuit.size()) oss << " -> ";
    }
    return oss.str();                                             // done
}

// -----------------------------
// Unified entry point: supports both undirected and directed graphs
// -----------------------------
std::string Euler::run(const Graph& g) {
    if (!g.directed()) {                                          // undirected mode
        return euler_undirected(g);                               // run the undirected routine
    } else {                                                      // directed mode
        return euler_directed(g);                                 // run the directed routine
    }
}
