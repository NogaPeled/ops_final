// ==========================
// Graph.cpp
// ==========================
// This file implements the out-of-line methods of the Graph class.
// Specifically: removeEdge(), reversed(), and label().
// All other methods are inline in Graph.hpp.
// ==========================

#include "graph/Graph.hpp"   // include the Graph class declaration
#include <sstream>           // used for building strings in label()

// --------------------------
// removeEdge
// --------------------------
// Purpose:
//   Remove a logical edge between u and v.
//   - For directed graphs: removes arc u->v only.
//   - For undirected graphs: removes both u->v and v->u, but counts once.
// Arguments:
//   u = source vertex index
//   v = destination vertex index
// Returns:
//   true if an edge was removed, false if nothing changed.
bool Graph::removeEdge(Vertex u, Vertex v) {
    checkIndex(u);                          // validate that u is within bounds
    checkIndex(v);                          // validate that v is within bounds

    bool changed = removeOneArc(u, v);      // try removing arc u->v
    if (!directed()) {                      // if the graph is undirected
        changed = removeOneArc(v, u) || changed; // also remove v->u, combine with previous result
    }

    // If any arc was removed, decrement logical edge counter
    if (changed && m_edgesLogical > 0) {
        --m_edgesLogical;                   // reduce logical edge count by one
    }

    return changed;                         // return true if something was removed
}

// --------------------------
// reversed
// --------------------------
// Purpose:
//   Build and return a new graph with reversed edges.
//   - For directed graphs: every arc u->v becomes v->u.
//   - For undirected graphs: adjacency is symmetric, so just copy.
// Returns:
//   A new Graph object with reversed adjacency.
Graph Graph::reversed() const {
    Graph rev(n(), m_kind, m_opts);         // create a new graph with same size and settings

    if (directed()) {                       // if graph is directed
        for (Vertex u = 0; u < n(); ++u) {  // iterate over all vertices
            for (const auto& e : m_adj[u]) {// iterate over adjacency of u
                // Insert reversed edge: instead of u->e.first, add e.first->u
                rev.m_adj[e.first].emplace_back(u, e.second);
            }
        }
    } else {
        // For undirected graphs, reversing has no effect
        rev.m_adj = m_adj;                  // copy adjacency as-is
    }

    rev.m_edgesLogical = m_edgesLogical;    // copy the logical edge count
    return rev;                             // return the reversed/copy graph
}

// --------------------------
// label
// --------------------------
// Purpose:
//   Produce a human-readable summary string of the graph.
// Format:
//   "DirectedGraph(VV,EE)" or "UndirectedGraph(VV,EE)"
//   where VV = number of vertices, EE = number of edges.
// Returns:
//   A std::string containing the description.
std::string Graph::label() const {
    std::ostringstream oss;                         // create a string stream
    oss << (directed() ? "Directed" : "Undirected");// write graph type
    oss << "Graph(" << n() << "V," << m() << "E)";  // add vertex and edge counts
    return oss.str();                               // return composed string
}
