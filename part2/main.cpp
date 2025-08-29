// ==========================
// Part 2: Euler circuit demo
// ==========================
// Here we create a small graph that has an Euler circuit
// and then call the Euler algorithm to verify and print it.
// ==========================

#include "graph/Graph.hpp"   // for Graph
#include "algo/Euler.hpp"    // for Euler
#include <iostream>          // for std::cout

int main() {
    // Create a simple 4-vertex cycle: 0–1–2–3–0.
    Graph g(4, Graph::Kind::Undirected);

    // Add edges to form a cycle of length 4.
    g.addEdge(0, 1, 1);
    g.addEdge(1, 2, 1);
    g.addEdge(2, 3, 1);
    g.addEdge(3, 0, 1);

    // Create the Euler solver object.
    Euler solver;

    // Run Euler circuit and print result.
    std::cout << solver.run(g) << "\n";

    return 0; // indicate success
}
