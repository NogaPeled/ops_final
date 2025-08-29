// ==========================
// Part 1: Graph demo
// ==========================
// In this part we only demonstrate that the Graph class
// can be constructed, edges added, and metadata printed.
// ==========================

#include "graph/Graph.hpp"   // include our Graph definition
#include <iostream>          // for std::cout

int main() {
    // Create an undirected graph with 4 vertices (0,1,2,3).
    Graph g(4, Graph::Kind::Undirected);

    // Add two edges: 0–1 and 1–2 with weight 1 each.
    g.addEdge(0, 1, 1);
    g.addEdge(1, 2, 1);

    // Print the label summary: "UndirectedGraph(4V,2E)".
    std::cout << g.label() << "\n";

    return 0; // indicate successful run
}
