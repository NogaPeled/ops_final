// ==========================
// tests/test_euler.cpp
// ==========================
// This file defines unit tests for the Graph and Euler classes.
// It uses the doctest framework so that we can run many small tests,
// see detailed reports, and measure code coverage.
// ==========================

// Enable doctest main entry point (so this file produces a `main()`)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"         // doctest framework header

// Include project headers
#include "graph/Graph.hpp"   // Graph class declaration
#include "algo/Euler.hpp"    // Euler algorithm class declaration

// Include for exception classes
#include <stdexcept>         // std::invalid_argument, std::out_of_range
#include <string>            // std::string

// ---------------------------
// Test 1: Undirected 4-cycle should have an Euler circuit
// ---------------------------
TEST_CASE("Undirected 4-cycle has an Euler circuit") {
    Graph g(4, Graph::Kind::Undirected);  // create graph with 4 vertices, undirected
    g.addEdge(0,1);                       // add edge 0-1
    g.addEdge(1,2);                       // add edge 1-2
    g.addEdge(2,3);                       // add edge 2-3
    g.addEdge(3,0);                       // add edge 3-0 (completes a cycle)
    Euler e;                              // construct Euler solver
    auto s = e.run(g);                    // run Euler algorithm
    CHECK(!s.empty());                    // result string should not be empty
    CHECK(s.find("Euler circuit") != std::string::npos); // must mention Euler circuit
}

// ---------------------------
// Test 2: Undirected with odd-degree vertex → no Euler circuit
// ---------------------------
TEST_CASE("Undirected with odd-degree vertex → no Euler circuit") {
    Graph g(3, Graph::Kind::Undirected);  // graph with 3 vertices
    g.addEdge(0,1);                       // edge 0-1
    g.addEdge(1,2);                       // edge 1-2
    Euler e;                              // solver
    auto s = e.run(g);                    // run
    CHECK(s.find("No Euler circuit") != std::string::npos); // expect failure message
}

// ---------------------------
// Test 3: Directed 3-cycle has an Euler circuit
// ---------------------------
TEST_CASE("Directed 3-cycle has a directed Euler circuit") {
    Graph g(3, Graph::Kind::Directed);    // directed graph with 3 vertices
    g.addEdge(0,1);                       // arc 0->1
    g.addEdge(1,2);                       // arc 1->2
    g.addEdge(2,0);                       // arc 2->0
    Euler e;                              // solver
    auto s = e.run(g);                    // run
    CHECK(s.find("Euler circuit (directed)") != std::string::npos); // expect success
}

// ---------------------------
// Test 4: Directed in/out mismatch → no Euler circuit
// ---------------------------
TEST_CASE("Directed in/out mismatch → no Euler circuit") {
    Graph g(3, Graph::Kind::Directed);    // 3 vertices
    g.addEdge(0,1);                       // arc 0->1
    g.addEdge(1,2);                       // arc 1->2
    g.addEdge(2,0);                       // arc 2->0
    g.addEdge(0,2);                       // extra arc 0->2 (out-degree of 0 is now 2)
    Euler e;                              // solver
    auto s = e.run(g);                    // run
    CHECK(s.find("in-degree != out-degree") != std::string::npos); // expect mismatch error
}

// ---------------------------
// Test 5: Directed graph with two SCCs → not strongly connected
// ---------------------------
TEST_CASE("Directed balanced but not strongly connected → no Euler circuit") {
    Graph g(4, Graph::Kind::Directed);    // 4 vertices
    g.addEdge(0,1); g.addEdge(1,0);       // SCC 1: 0 <-> 1
    g.addEdge(2,3); g.addEdge(3,2);       // SCC 2: 2 <-> 3
    Euler e;                              // solver
    auto s = e.run(g);                    // run
    CHECK(s.find("not strongly connected") != std::string::npos); // expect failure
}

// ---------------------------
// Test 6: Graph::reversed on directed graphs
// ---------------------------
TEST_CASE("Graph::reversed on directed") {
    Graph g(3, Graph::Kind::Directed);    // directed graph
    g.addEdge(0,1,5);                     // 0->1 with weight 5
    g.addEdge(2,1,7);                     // 2->1 with weight 7
    Graph r = g.reversed();               // reversed graph
    CHECK(r.hasArc(1,0));                 // reversed contains 1->0
    CHECK(r.hasArc(1,2));                 // reversed contains 1->2
    CHECK_FALSE(r.hasArc(0,1));           // original direction 0->1 not present
}

// ---------------------------
// Test 7: Graph::removeEdge on undirected
// ---------------------------
TEST_CASE("Graph::removeEdge on undirected removes both arcs") {
    Graph g(3, Graph::Kind::Undirected);  // undirected triangle
    g.addEdge(0,1); g.addEdge(1,2); g.addEdge(2,0);
    CHECK(g.removeEdge(1,2));             // remove edge 1-2
    CHECK_FALSE(g.hasArc(1,2));           // ensure arc 1->2 removed
    CHECK_FALSE(g.hasArc(2,1));           // ensure reverse arc 2->1 removed too
}

// ---------------------------
// Test 8: Graph options (no self-loops, no multi-edges)
// ---------------------------
TEST_CASE("Graph options: no self-loops, no multi-edges") {
    Graph::Options opt;                   // options struct
    opt.allowSelfLoops = false;           // disable self-loops
    opt.allowMultiEdges = false;          // disable parallel edges
    Graph g(3, Graph::Kind::Undirected, opt);
    g.addEdge(0,1);                       // add 0-1
    g.addEdge(0,1);                       // duplicate should be ignored
    CHECK(g.adj(0).size() == 1);          // only one stored
    CHECK_THROWS_AS(g.addEdge(2,2), std::invalid_argument); // self-loop throws
}

// ---------------------------
// Test 9: Graph::label()
// ---------------------------
TEST_CASE("Graph::label() non-empty and mentions type") {
    Graph g(2, Graph::Kind::Undirected);  // 2 vertices, undirected
    g.addEdge(0,1);                       // add edge
    auto L = g.label();                   // call label()
    CHECK(!L.empty());                    // must not be empty
    CHECK(L.find("UndirectedGraph(") != std::string::npos); // must mention type
}

// ---------------------------
// Test 10: reversed() on undirected should be identical
// ---------------------------
TEST_CASE("reversed() on undirected copies adjacency") {
    Graph g(3, Graph::Kind::Undirected);
    g.addEdge(0,1); g.addEdge(1,2);
    Graph r = g.reversed();               // reversed graph
    CHECK(r.hasArc(0,1));                 // original edges still there
    CHECK(r.hasArc(1,0));
    CHECK(r.hasArc(1,2));
    CHECK(r.hasArc(2,1));
}

// ---------------------------
// Test 11: removeEdge non-existent → returns false
// ---------------------------
TEST_CASE("removeEdge() non-existent → returns false") {
    Graph g(3, Graph::Kind::Undirected);
    g.addEdge(0,1);                       // only edge
    CHECK_FALSE(g.removeEdge(1,2));       // removing missing edge should fail
    CHECK(g.hasArc(0,1));                 // 0-1 still present
    CHECK(g.hasArc(1,0));                 // 1-0 still present
}

// ---------------------------
// Test 12: adj() invalid index throws out_of_range
// ---------------------------
TEST_CASE("adj() out_of_range throws") {
    Graph g(2, Graph::Kind::Undirected);  // vertices 0 and 1
    CHECK_THROWS_AS((void)g.adj(2), std::out_of_range); // invalid index 2
}

// ---------------------------
// Test 13: trivial undirected graph (no edges)
// ---------------------------
TEST_CASE("Trivial: undirected no-edges message") {
    Graph g(3, Graph::Kind::Undirected);  // no edges
    Euler e;
    auto s = e.run(g);
    CHECK(s.find("Graph has no edges") != std::string::npos);
}

// ---------------------------
// Test 14: trivial directed graph (no edges)
// ---------------------------
TEST_CASE("Trivial: directed no-edges message") {
    Graph g(3, Graph::Kind::Directed);    // no edges
    Euler e;
    auto s = e.run(g);
    CHECK(s.find("Graph has no edges") != std::string::npos);
}

TEST_CASE("Undirected: even degrees but disconnected -> no Euler") {
    Graph g(6, Graph::Kind::Undirected);
    // two disjoint 3-cycles: 0-1-2-0 and 3-4-5-3 (all degrees = 2, but disconnected)
    g.addEdge(0,1); g.addEdge(1,2); g.addEdge(2,0);
    g.addEdge(3,4); g.addEdge(4,5); g.addEdge(5,3);
    Euler e;
    auto s = e.run(g);
    CHECK(s.find("disconnected on non-isolated vertices") != std::string::npos);
}

// Self-loop is DISALLOWED by default -> should throw
TEST_CASE("Undirected: adding a self-loop throws when disabled") {
    Graph g(1, Graph::Kind::Undirected);          // default: allowSelfLoops = false
    CHECK_THROWS_AS(g.addEdge(0,0), std::invalid_argument);
}

// Self-loop is ALLOWED via options -> Euler circuit should succeed
TEST_CASE("Undirected: self-loop allowed yields a valid Euler circuit") {
    Graph::Options opt;
    opt.allowSelfLoops = true;                     // enable self-loops
    opt.allowMultiEdges = false;
    Graph g(1, Graph::Kind::Undirected, opt);
    g.addEdge(0,0);                                // now OK
    Euler e;
    auto s = e.run(g);
    CHECK(s.find("Euler circuit") != std::string::npos);
}


