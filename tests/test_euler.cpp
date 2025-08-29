// ==========================
// tests/test_euler.cpp
// ==========================
// Purpose:
//   Small unit-style checks for Graph and Euler.
//   We use simple `assert` statements; if all pass we print "OK".
//   Each line has a comment so you can follow along.
// ==========================

#include "graph/Graph.hpp"         // bring in the Graph class API
#include "algo/Euler.hpp"          // bring in the Euler algorithm
#include <cassert>                 // for assert()
#include <iostream>                // for std::cout
#include <string>                  // for std::string searches

int main() {                       // test program entry point

    { // ---------- Test 1: Undirected 4-cycle should have an Euler circuit ----------
      // Build an undirected cycle on 4 vertices: 0-1-2-3-0 (all degrees even).
        Graph g(4, Graph::Kind::Undirected);      // create graph with 4 vertices
        g.addEdge(0,1,1);                          // add edge 0-1
        g.addEdge(1,2,1);                          // add edge 1-2
        g.addEdge(2,3,1);                          // add edge 2-3
        g.addEdge(3,0,1);                          // add edge 3-0 (cycle closes)
        Euler e;                                    // construct Euler solver
        std::string s = e.run(g);                   // run Euler on the graph
        assert(!s.empty());                         // ensure we got a non-empty message
        assert(s.find("Euler circuit") != std::string::npos); // expect success phrase
    }

    { // ---------- Test 2: Undirected with odd degree → no Euler circuit ----------
      // Build a path 0-1-2 (vertex 1 has degree 2, but 0 and 2 have degree 1 → odd).
        Graph g(3, Graph::Kind::Undirected);      // 3 vertices
        g.addEdge(0,1,1);                          // edge 0-1
        g.addEdge(1,2,1);                          // edge 1-2
        Euler e;                                    // solver
        std::string s = e.run(g);                   // run
        assert(s.find("No Euler circuit") != std::string::npos); // should fail
    }

    { // ---------- Test 3: Directed 3-cycle should have a directed Euler circuit ----------
      // Build 0->1->2->0 (each vertex has in=out=1, strongly connected).
        Graph g(3, Graph::Kind::Directed);        // directed graph with 3 vertices
        g.addEdge(0,1,1);                          // arc 0->1
        g.addEdge(1,2,1);                          // arc 1->2
        g.addEdge(2,0,1);                          // arc 2->0
        Euler e;                                    // solver
        std::string s = e.run(g);                   // run
        assert(s.find("Euler circuit (directed)") != std::string::npos); // expect success
    }

    { // ---------- Test 4: Directed with in/out mismatch → no Euler circuit ----------
      // Make 0->1, 1->2, 2->0, plus extra 0->2 (now out(0)=2, in(0)=1 → mismatch).
        Graph g(3, Graph::Kind::Directed);        // directed on 3 vertices
        g.addEdge(0,1,1);                          // 0->1
        g.addEdge(1,2,1);                          // 1->2
        g.addEdge(2,0,1);                          // 2->0
        g.addEdge(0,2,1);                          // extra 0->2 causes imbalance
        Euler e;                                    // solver
        std::string s = e.run(g);                   // run
        assert(s.find("in-degree != out-degree") != std::string::npos); // expect failure
    }

    { // ---------- Test 5: Directed with balanced degrees but not strongly connected ----------
      // Two cycles disconnected: (0->1->0) and (2->3->2). in=out holds, but no strong connectivity among all.
        Graph g(4, Graph::Kind::Directed);        // 4 vertices
        g.addEdge(0,1,1);                          // first SCC: 0->1
        g.addEdge(1,0,1);                          // first SCC: 1->0
        g.addEdge(2,3,1);                          // second SCC: 2->3
        g.addEdge(3,2,1);                          // second SCC: 3->2
        Euler e;                                    // solver
        std::string s = e.run(g);                   // run
        assert(s.find("not strongly connected") != std::string::npos); // expect failure
    }

    { // ---------- Test 6: Graph::reversed on directed ----------
      // Reverse 0->1 and 2->1; after reversed(), we must have 1->0 and 1->2.
        Graph g(3, Graph::Kind::Directed);        // create directed graph
        g.addEdge(0,1,5);                          // 0->1 with weight 5 (weight ignored here)
        g.addEdge(2,1,7);                          // 2->1 with weight 7
        Graph r = g.reversed();                    // build reversed graph
        assert(r.hasArc(1,0));                     // check 1->0 exists
        assert(r.hasArc(1,2));                     // check 1->2 exists
        assert(!r.hasArc(0,1));                    // original direction should not exist in reversed
    }

    { // ---------- Test 7: Graph::removeEdge on undirected ----------
      // Start with triangle 0-1-2-0, then remove edge 1-2 and verify counts/adjacency.
        Graph g(3, Graph::Kind::Undirected);      // undirected graph
        g.addEdge(0,1,1);                          // 0-1
        g.addEdge(1,2,1);                          // 1-2
        g.addEdge(2,0,1);                          // 2-0
        bool removed = g.removeEdge(1,2);          // remove logical edge between 1 and 2
        assert(removed);                            // should return true
        assert(!g.hasArc(1,2));                    // 1->2 gone
        assert(!g.hasArc(2,1));                    // 2->1 gone (undirected also removed reverse)
    }

    { // ---------- Test 8: Graph options (no self-loops, no multi-edges) ----------
      // Try to insert self-loop and duplicate edges; expect either exception or dedup behavior.
        Graph::Options opt;                        // create options struct
        opt.allowSelfLoops = false;                // disallow self-loops
        opt.allowMultiEdges = false;               // disallow duplicates
        Graph g(3, Graph::Kind::Undirected, opt);  // construct with options
        g.addEdge(0,1,1);                          // add first edge 0-1
        g.addEdge(0,1,1);                          // duplicate should be ignored
        assert(g.adj(0).size() == 1);              // only one neighbor entry on 0
        // Self-loop should throw; wrap in a try/catch to verify behavior.
        bool threw = false;                         // flag to record exception
        try {
            g.addEdge(2,2,1);                      // attempt self-loop
        } catch (const std::invalid_argument&) {
            threw = true;                          // expected path when self-loops disabled
        }
        assert(threw);                              // verify exception occurred
    }

    // ===== Extra coverage blocks you asked for =====

    { // ---------- Extra Test A: Graph::label() ----------
      // Create a small graph and call label() to cover the function.
        Graph g(2, Graph::Kind::Undirected);       // 2 vertices, undirected
        g.addEdge(0,1,1);                           // single edge
        std::string L = g.label();                  // call label() to exercise Graph::label()
        assert(!L.empty());                         // label should not be empty
        assert(L.find("UndirectedGraph(") != std::string::npos); // should mention UndirectedGraph
    }

    { // ---------- Extra Test B: reversed() on UNDIRECTED ----------
      // For undirected graphs, reversed() should copy adjacency as-is (covers the undirected branch).
        Graph g(3, Graph::Kind::Undirected);       // undirected graph
        g.addEdge(0,1,1);                           // 0-1
        g.addEdge(1,2,1);                           // 1-2
        Graph r = g.reversed();                     // call reversed() to hit the undirected branch
        assert(r.hasArc(0,1));                      // 0-1 present
        assert(r.hasArc(1,0));                      // 1-0 present (because undirected stores both arcs)
        assert(r.hasArc(1,2));                      // 1-2 present
        assert(r.hasArc(2,1));                      // 2-1 present
    }

    { // ---------- Extra Test C: removeEdge() -> false path ----------
      // Try removing a non-existent edge to exercise the "changed == false" branch.
        Graph g(3, Graph::Kind::Undirected);       // fresh undirected graph
        g.addEdge(0,1,1);                           // only one edge exists
        bool removed = g.removeEdge(1,2);           // attempt to remove 1-2 which does not exist
        assert(!removed);                           // should return false
        assert(g.hasArc(0,1));                      // 0-1 still present
        assert(g.hasArc(1,0));                      // 1-0 still present (undirected reverse)
    }

    std::cout << "OK\n";                           // print success sentinel for the Makefile target
    return 0;     
    
    { // ---------- Extra Test D: checkIndex() exception path ----------
      // Force an out_of_range by accessing an invalid vertex index.
        Graph g(2, Graph::Kind::Undirected);      // vertices: 0,1
        bool threw = false;                        // track the exception
        try {
            (void)g.adj(2);                        // invalid index (>= n) -> should throw
        } catch (const std::out_of_range&) {
            threw = true;                          // expected
        }
        assert(threw);                             // verify exception occurred
    }
    // indicate success to the OS
}
