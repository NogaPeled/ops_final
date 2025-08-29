#pragma once                              // ensure this header is included only once per translation unit

#include <vector>        // used for adjacency lists
#include <utility>       // used for std::pair to represent edges
#include <cstddef>       // defines std::size_t type
#include <stdexcept>     // defines exceptions like out_of_range, invalid_argument
#include <algorithm>     // used for std::any_of and std::find_if
#include <string>        // used for std::string in label()

// ==========================
// Minimal, future-proof Graph
// ==========================
// This class supports:
// - Directed and undirected graphs
// - Weights on edges (for MST, Max-Flow, etc.)
// - Const adjacency access (for Euler, Hamilton, SCC algorithms)
// - reversed() builder (for SCC and flow algorithms)
// - Guards against self-loops and multi-edges (for simple graphs)
// ==========================

class Graph {
public:
    // Enumeration to specify whether the graph is Undirected or Directed
    enum class Kind { Undirected, Directed };

    // Options to control behavior for self-loops and multi-edges
    struct Options {
        bool allowSelfLoops  = false; // if false, edges u->u are forbidden
        bool allowMultiEdges = false; // if false, parallel edges are forbidden
    };

    // Type aliases for readability
    using Vertex = std::size_t;             // vertex index type
    using Weight = long long;               // edge weight or capacity type
    using Edge   = std::pair<Vertex, Weight>; // edge represented as (neighbor, weight)

    // ---- Constructors ----

    // Primary constructor taking explicit Options
    explicit Graph(std::size_t n, Kind kind, Options opts)
        : m_kind(kind), m_opts(opts), m_adj(n), m_edgesLogical(0) {}

    // Convenience constructor: uses default Options{}
    explicit Graph(std::size_t n = 0, Kind kind = Kind::Undirected)
        : m_kind(kind), m_opts(Options{}), m_adj(n), m_edgesLogical(0) {}

    // ---- Public API ----

    // Return the number of vertices
    std::size_t n() const noexcept { return m_adj.size(); }

    // Return whether the graph is Undirected or Directed
    Kind kind() const noexcept { return m_kind; }

    // Convenience: return true if the graph is Directed
    bool directed() const noexcept { return m_kind == Kind::Directed; }

    // Return the number of logical edges
    std::size_t m() const noexcept { return m_edgesLogical; }

    // Access adjacency list of vertex `u`
    const std::vector<Edge>& adj(Vertex u) const {
        checkIndex(u);
        return m_adj[u];
    }

    // Add edge u->v with optional weight w (default = 1)
    void addEdge(Vertex u, Vertex v, Weight w = 1) {
        checkIndex(u);
        checkIndex(v);

        if (!m_opts.allowSelfLoops && u == v) {
            throw std::invalid_argument("self-loops are disabled in this graph");
        }

        if (!m_opts.allowMultiEdges) {
            if (hasArc(u, v)) return;
            if (!directed() && hasArc(v, u)) return;
        }

        m_adj[u].emplace_back(v, w);

        if (!directed()) {
            m_adj[v].emplace_back(u, w);
        }

        ++m_edgesLogical;
    }

    // Remove logical edge between u and v (implemented in Graph.cpp)
    bool removeEdge(Vertex u, Vertex v);

    // Compute out-degree of each vertex
    std::vector<std::size_t> outDegree() const {
        std::vector<std::size_t> d(n());
        for (Vertex u = 0; u < n(); ++u) d[u] = m_adj[u].size();
        return d;
    }

    // Compute in-degree of each vertex
    std::vector<std::size_t> inDegree() const {
        std::vector<std::size_t> d(n(), 0);
        for (Vertex u = 0; u < n(); ++u)
            for (const auto& e : m_adj[u]) ++d[e.first];
        return d;
    }

    // Compute degree (only for undirected graphs)
    std::vector<std::size_t> degree() const {
        if (directed())
            throw std::logic_error("degree() is defined for undirected graphs only");
        std::vector<std::size_t> d(n());
        for (Vertex u = 0; u < n(); ++u) d[u] = m_adj[u].size();
        return d;
    }

    // Return true if arc u->v exists
    bool hasArc(Vertex u, Vertex v) const {
        checkIndex(u);
        checkIndex(v);
        const auto& lst = m_adj[u];
        return std::any_of(lst.begin(), lst.end(),
                           [v](const Edge& e){ return e.first == v; });
    }

    // Build and return a reversed graph (implemented in Graph.cpp)
    Graph reversed() const;

    // Return a human-readable summary of the graph (implemented in Graph.cpp)
    std::string label() const;

private:
    Kind m_kind;                               // directed or undirected
    Options m_opts;                            // options (loops, multi-edges)
    std::vector<std::vector<Edge>> m_adj;      // adjacency list
    std::size_t m_edgesLogical;                // number of logical edges

    // Helper: check if vertex index is valid
    void checkIndex(Vertex u) const {
        if (u >= m_adj.size())
            throw std::out_of_range("vertex index out of range");
    }

    // Helper: remove arc u->v from adjacency list of u
    bool removeOneArc(Vertex u, Vertex v) {
        auto& lst = m_adj[u];
        auto it = std::find_if(lst.begin(), lst.end(),
                               [v](const Edge& e){ return e.first == v; });
        if (it != lst.end()) {
            lst.erase(it);
            return true;
        }
        return false;
    }
}; // end class Graph
