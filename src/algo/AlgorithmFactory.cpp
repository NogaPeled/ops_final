// ===============================================
// AlgorithmFactory.cpp
// Implements four algorithms (Strategy pattern):
//   * MST weight (Kruskal; undirected only; error if disconnected)
//   * SCC count (Kosaraju; works best for directed graphs)
//   * Max flow (Edmonds–Karp) from 0 to n-1
//   * Hamiltonian circuit existence (backtracking)
// Exposes AlgorithmFactory::create(name) to instantiate a strategy.
// Defines/implements the factory.
// ===============================================

#include "../include/algo/GraphAlgorithm.hpp"    // Include the interface and factory declaration.
#include <algorithm>                  // std::sort, std::minmax
#include <cctype>                     // std::tolower for case-insensitive names
#include <climits>                    // LLONG_MAX for max-flow bottleneck
#include <memory>                     // std::make_unique for factory
#include <queue>                      // std::queue used by BFS in max-flow
#include <set>                        // std::set to dedupe edges
#include <sstream>                    // std::ostringstream to build responses
#include <string>                     // std::string
#include <vector>                     // std::vector

// ---------- helper: to-lower a string (safe cast to unsigned char) ----------
static std::string to_lower(std::string s) {                       // Copy input string.
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); // Lowercase each byte.
    return s;                                                      // Return transformed string.
}

// =====================================================
// 1) MST weight (Kruskal) — undirected graphs only
// =====================================================
struct AlgoMstWeight final : IGraphAlgorithm {                     // Concrete strategy type.
    std::string run(const Graph& g) override {                     // Main entry point for the MST algorithm.
        if (g.directed())                                          // If graph is directed…
            return "MST undefined for directed graphs.";           // …we consider MST undefined.

        const std::size_t n = g.n();                               // Number of vertices.
        if (n == 0) return "MST weight: 0 (empty graph).";         // Trivial case: empty graph has weight 0.

        struct Edge { int w; int u; int v; };                      // Simple record to hold (weight, u, v).
        std::vector<Edge> edges;                                   // Container for unique undirected edges.
        edges.reserve(g.m());                                      // Reserve roughly number of edges.

        for (Graph::Vertex u = 0; u < n; ++u) {                    // For each vertex u…
            for (const auto& e : g.adj(u)) {                       // …scan adjacency list of (v, w).
                int v = static_cast<int>(e.first);                 // Neighbor vertex id.
                int w = static_cast<int>(e.second);                // Edge weight (or 0 if unweighted).
                if (u < static_cast<std::size_t>(v))               // Only keep one direction (u < v) to avoid duplicates.
                    edges.push_back({w, static_cast<int>(u), v});  // Store the edge.
            }
        }

        std::sort(edges.begin(), edges.end(),                      // Sort edges by non-decreasing weight.
                  [](const Edge& a, const Edge& b){ return a.w < b.w; });

        std::vector<int> parent(n), rnk(n, 0);                     // Disjoint-set union (parent and rank).
        for (std::size_t i = 0; i < n; ++i) parent[i] = static_cast<int>(i); // Initialize each node as its own parent.

        auto find = [&](auto&& self, int x) -> int {               // Path-compressed find.
            return parent[x] == x ? x : parent[x] = self(self, parent[x]);
        };

        auto unite = [&](int a, int b) -> bool {                    // Union by rank; return true if merged.
            a = find(find, a);                                      // Find root of a.
            b = find(find, b);                                      // Find root of b.
            if (a == b) return false;                               // Already in same set → no merge.
            if (rnk[a] < rnk[b]) std::swap(a, b);                   // Ensure a has >= rank.
            parent[b] = a;                                          // Make a the parent of b.
            if (rnk[a] == rnk[b]) ++rnk[a];                         // Increase rank if equal.
            return true;                                            // Report a successful merge.
        };

        long long total = 0;                                        // Accumulator for MST weight.
        std::size_t used = 0;                                       // Number of edges chosen.
        for (const auto& ed : edges) {                              // Iterate edges in weight order.
            if (unite(ed.u, ed.v)) {                                // If this edge connects two components…
                total += ed.w;                                      // …add its weight to MST total.
                ++used;                                             // Increment count of chosen edges.
                if (used == n - 1) break;                           // Early exit when MST has n-1 edges.
            }
        }

        if (used != n - 1)                                          // If we didn’t connect all vertices…
            return "Graph is disconnected; MST does not exist.";    // …there’s no spanning tree.

        std::ostringstream oss;                                     // Build a human-readable message.
        oss << "MST weight: " << total << " (edges used: " << used << ")."; // Include count for clarity.
        return oss.str();                                           // Return message.
    }
};

// ================================================
// 2) SCC count (Kosaraju) — for directed graphs
// ================================================
struct AlgoSccCount final : IGraphAlgorithm {                       // Concrete strategy type.
    std::string run(const Graph& g) override {                      // Main entry for SCC counting.
        const std::size_t n = g.n();                                // Number of vertices.
        if (n == 0) return "SCC count: 0 (empty graph).";           // Trivial for empty graph.

        std::vector<char> seen(n, 0);                               // Visited flags for first DFS.
        std::vector<Graph::Vertex> order; order.reserve(n);         // Finish order list.

        auto dfs1 = [&](auto&& self, Graph::Vertex u) -> void {     // First pass DFS (on g).
            seen[u] = 1;                                            // Mark as visited.
            for (const auto& e : g.adj(u)) {                        // For each edge u→v…
                Graph::Vertex v = e.first;                          // Extract neighbor.
                if (!seen[v]) self(self, v);                        // Recurse if not seen.
            }
            order.push_back(u);                                     // Record vertex by finish time.
        };

        for (Graph::Vertex u = 0; u < n; ++u)                       // Start DFS from every unvisited vertex.
            if (!seen[u]) dfs1(dfs1, u);                            // Recurse.

        Graph gr = g.reversed();                                    // Reverse the graph for second pass.
        std::fill(seen.begin(), seen.end(), 0);                     // Reset visited flags.
        std::size_t comps = 0;                                      // Component counter.

        auto dfs2 = [&](auto&& self, Graph::Vertex u) -> void {     // Second pass DFS (on reversed graph).
            seen[u] = 1;                                            // Mark visited.
            for (const auto& e : gr.adj(u)) {                       // For each reverse edge u→v…
                Graph::Vertex v = e.first;                          // Extract neighbor.
                if (!seen[v]) self(self, v);                        // Recurse if not seen.
            }
        };

        for (std::size_t i = 0; i < n; ++i) {                       // Process vertices in reverse finish order.
            Graph::Vertex u = order[n - 1 - i];                     // Take from the end of order.
            if (!seen[u]) {                                         // If this root not seen…
                ++comps;                                            // …we found a new SCC.
                dfs2(dfs2, u);                                      // Mark entire component.
            }
        }

        std::ostringstream oss;                                     // Build message.
        oss << "SCC count: " << comps << ".";                       // Include count.
        return oss.str();                                           // Return.
    }
};

// ==========================================================
// 3) Max flow (Edmonds–Karp) from source 0 to sink n-1
// ==========================================================
struct AlgoMaxFlow final : IGraphAlgorithm {                         // Concrete strategy type.
    std::string run(const Graph& g) override {                       // Entry point for max flow.
        const std::size_t n = g.n();                                 // Number of vertices.
        if (n < 2) return "Max flow: 0 (need at least two vertices)."; // Need source and sink.

        std::vector<std::vector<long long>> cap(                     // Residual capacity matrix.
            n, std::vector<long long>(n, 0));                        // Initialize all capacities to zero.

        for (Graph::Vertex u = 0; u < n; ++u) {                 // for each vertex u
            for (const auto& e : g.adj(u)) {                    // for each outgoing edge u->v
                Graph::Vertex v = e.first;                      // neighbor vertex id
                long long w = e.second ? static_cast<long long>(e.second) : 1LL;
                cap[u][v] += w;                                 // add forward capacity

                // IMPORTANT: do NOT mirror for undirected graphs here.
                // Your Graph exposes undirected edges in BOTH adjacency lists,
                // so when the outer loop reaches 'v', you'll naturally add cap[v][u].
            }
        }

        Graph::Vertex s = 0;                                         // Source is node 0.
        Graph::Vertex t = static_cast<Graph::Vertex>(n - 1);         // Sink is node n-1.
        long long flow = 0;                                          // Accumulated max flow.

        while (true) {                                               // Repeat until no augmenting path exists.
            std::vector<int> parent(n, -1);                          // To reconstruct path.
            std::queue<Graph::Vertex> q;                             // BFS queue.
            parent[s] = static_cast<int>(s);                         // Mark source as visited (parent to itself).
            q.push(s);                                               // Start BFS from source.

            while (!q.empty() && parent[t] == -1) {                  // While sink not reached…
                Graph::Vertex u = q.front(); q.pop();                // Pop next vertex.
                for (Graph::Vertex v = 0; v < n; ++v) {              // Examine all possible neighbors.
                    if (parent[v] == -1 && cap[u][v] > 0) {          // If residual capacity exists and v not visited…
                        parent[v] = static_cast<int>(u);             // Record predecessor on path.
                        q.push(v);                                   // Enqueue v.
                        if (v == t) break;                           // Early exit if we reached sink.
                    }
                }
            }

            if (parent[t] == -1) break;                              // No augmenting path → we’re done.

            long long add = LLONG_MAX;                               // Bottleneck capacity along the path.
            for (int v = static_cast<int>(t); v != static_cast<int>(s); v = parent[v]) { // Walk back sink→source.
                int u = parent[v];                                   // Predecessor on path.
                add = std::min(add, cap[u][v]);                      // Take minimum residual capacity.
            }

            for (int v = static_cast<int>(t); v != static_cast<int>(s); v = parent[v]) { // Update residual graph.
                int u = parent[v];                                   // Predecessor.
                cap[u][v] -= add;                                    // Reduce forward capacity.
                cap[v][u] += add;                                    // Increase backward capacity.
            }

            flow += add;                                             // Increase total max flow.
        }

        std::ostringstream oss;                                      // Build output string.
        oss << "Max flow (0 -> " << (n - 1) << "): " << flow << "."; // Include source/sink in message.
        return oss.str();                                            // Return.
    }
};

// ==================================================================
// 4) Hamiltonian circuit (cycle) existence via backtracking
// ==================================================================
struct AlgoHamilton final : IGraphAlgorithm {                         // Concrete strategy type.
    std::string run(const Graph& g) override {                        // Entry point for Hamiltonian cycle.
        const std::size_t n = g.n();                                  // Number of vertices.
        if (n == 0) return "Hamiltonian circuit: trivial (empty).";   // Empty graph message.
        if (n == 1) return "Hamiltonian circuit: 0 -> 0";             // Single node cycle.

        std::vector<std::vector<char>> A(                              // Adjacency matrix for O(1) checks.
            n, std::vector<char>(n, 0));                               // Initialize to no edges.

        for (Graph::Vertex u = 0; u < n; ++u) {                        // For each vertex…
            for (const auto& e : g.adj(u)) {                           // …scan adjacency list.
                Graph::Vertex v = e.first;                             // Neighbor vertex id.
                A[u][v] = 1;                                           // Directed arc u→v exists.
                if (!g.directed()) A[v][u] = 1;                        // Mirror for undirected graphs.
            }
        }

        std::vector<Graph::Vertex> path; path.reserve(n + 1);          // Sequence of vertices forming the cycle.
        std::vector<char> used(n, 0);                                  // Visited flags for path.
        Graph::Vertex start = 0;                                       // Start at vertex 0 (convention).
        path.push_back(start);                                         // Put start in the path.
        used[start] = 1;                                               // Mark start as used.

        bool found = false;                                            // Flag to stop early once found.

        auto dfs = [&](auto&& self, Graph::Vertex u) -> void {         // Backtracking DFS.
            if (found) return;                                         // Stop recursion if already found.
            if (path.size() == n) {                                    // If we placed all vertices…
                if (A[u][start]) {                                     // …and there is an edge back to start…
                    path.push_back(start);                              // …close the cycle.
                    found = true;                                      // Mark success.
                }
                return;                                                // Return regardless.
            }
            for (Graph::Vertex v = 0; v < n; ++v) {                    // Try all possible next vertices.
                if (!used[v] && A[u][v]) {                             // Must be unused and adjacent.
                    used[v] = 1;                                       // Mark as used.
                    path.push_back(v);                                 // Extend path.
                    self(self, v);                                     // Recurse deeper.
                    if (found) return;                                 // If found, bubble out.
                    path.pop_back();                                   // Otherwise, undo choice.
                    used[v] = 0;                                       // Unmark to try others.
                }
            }
        };

        dfs(dfs, start);                                               // Start DFS from start vertex.

        if (!found) return "No Hamiltonian circuit.";                  // Report if none found.

        std::ostringstream oss;                                        // Build readable cycle.
        oss << "Hamiltonian circuit: ";                                // Header text.
        for (std::size_t i = 0; i < path.size(); ++i) {                // Print vertices in order.
            oss << path[i];                                            // Vertex id.
            if (i + 1 < path.size()) oss << " -> ";                    // Arrow between vertices.
        }
        return oss.str();                                              // Return message.
    }
};

// =====================================================
// Factory definition (matches header declaration)
// =====================================================
std::unique_ptr<IGraphAlgorithm>                                   // Return unique_ptr to created strategy.
AlgorithmFactory::create(const std::string& name) {                // Define factory method declared in header.
    const auto n = to_lower(name);                                  // Normalize the name to lowercase.
    if (n == "mst")      return std::make_unique<AlgoMstWeight>();  // Create MST strategy.
    if (n == "scc")      return std::make_unique<AlgoSccCount>();   // Create SCC strategy.
    if (n == "maxflow")  return std::make_unique<AlgoMaxFlow>();    // Create Max Flow strategy.
    if (n == "hamilton") return std::make_unique<AlgoHamilton>();   // Create Hamiltonian strategy.
    return nullptr;                                                 // Unknown name → caller handles error.
}
