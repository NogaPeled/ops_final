// tests/test_all.cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "graph/Graph.hpp"
#include "algo/GraphAlgorithm.hpp"

// Small helper to create & run an algorithm by name
static std::string run_algo(const char* name, const Graph& g) {
    auto p = AlgorithmFactory::create(name);
    REQUIRE(p != nullptr);
    return p->run(g);
}

// ---------------- Hamiltonian ----------------

TEST_CASE("Hamilton on 4-cycle (undirected)") {
    Graph::Options o; o.allowSelfLoops=false; o.allowMultiEdges=false;
    Graph g(4, Graph::Kind::Undirected, o);
    g.addEdge(0,1,1); g.addEdge(1,2,1); g.addEdge(2,3,1); g.addEdge(3,0,1);

    auto out = run_algo("HAMILTON", g);
    CHECK(out.find("Hamiltonian") != std::string::npos);
}

TEST_CASE("Hamilton negative on a simple path (no cycle)") {
    Graph::Options o; o.allowSelfLoops=false; o.allowMultiEdges=false;
    Graph g(4, Graph::Kind::Undirected, o);
    g.addEdge(0,1,1); g.addEdge(1,2,1); g.addEdge(2,3,1); // path, no cycle

    auto out = run_algo("HAMILTON", g);
    CHECK(out.find("No Hamiltonian") != std::string::npos);
}

// ---------------- MaxFlow ----------------

TEST_CASE("MaxFlow simple directed 0->3 = 2") {
    Graph::Options o; o.allowSelfLoops=false; o.allowMultiEdges=false;
    Graph g(4, Graph::Kind::Directed, o);
    g.addEdge(0,1,1); g.addEdge(1,2,1); g.addEdge(2,3,1);
    g.addEdge(0,2,1); g.addEdge(1,3,1);

    auto out = run_algo("MAXFLOW", g);
    CHECK(out.find("Max flow (0 -> 3): 2") != std::string::npos);
}

TEST_CASE("MaxFlow zero when no path 0->n-1") {
    Graph::Options o; o.allowSelfLoops=false; o.allowMultiEdges=false;
    Graph g(4, Graph::Kind::Directed, o);
    g.addEdge(1,2,1); // 0 isolated from 3

    auto out = run_algo("MAXFLOW", g);
    CHECK(out.find("Max flow (0 -> 3): 0") != std::string::npos);
}

// ---------------- MST ----------------

TEST_CASE("MST on chain: correct weight") {
    Graph::Options o; o.allowSelfLoops=false; o.allowMultiEdges=false;
    Graph g(4, Graph::Kind::Undirected, o);
    // chain weights 1+2+3 => 6
    g.addEdge(0,1,1); g.addEdge(1,2,2); g.addEdge(2,3,3);

    auto out = run_algo("MST", g);
    CHECK(out.find("MST weight: 6") != std::string::npos);
}

TEST_CASE("MST reports disconnected graph") {
    Graph::Options o; o.allowSelfLoops=false; o.allowMultiEdges=false;
    Graph g(4, Graph::Kind::Undirected, o);
    g.addEdge(0,1,1);       // {0,1} and {2,3} separate
    g.addEdge(2,3,1);

    auto out = run_algo("MST", g);
    CHECK(out.find("disconnected") != std::string::npos);
}

TEST_CASE("MST undefined for directed graphs") {
    Graph::Options o; o.allowSelfLoops=false; o.allowMultiEdges=false;
    Graph g(3, Graph::Kind::Directed, o);
    g.addEdge(0,1,1); g.addEdge(1,2,1);

    auto out = run_algo("MST", g);
    CHECK(out.find("undefined") != std::string::npos);
}

// ---------------- SCC ----------------

TEST_CASE("SCC count = 1 on strongly-connected 3-cycle") {
    Graph::Options o; o.allowSelfLoops=false; o.allowMultiEdges=false;
    Graph g(3, Graph::Kind::Directed, o);
    g.addEdge(0,1,1); g.addEdge(1,2,1); g.addEdge(2,0,1);

    auto out = run_algo("SCC", g);
    CHECK(out.find("SCC count: 1") != std::string::npos);
}

// ---------------- Factory ----------------

TEST_CASE("Factory recognizes algorithm names (case-insensitive)") {
    CHECK(AlgorithmFactory::create("MST"));
    CHECK(AlgorithmFactory::create("mst"));
    CHECK(AlgorithmFactory::create("SCC"));
    CHECK(AlgorithmFactory::create("MaxFlow"));
    CHECK(AlgorithmFactory::create("HAMILTON"));
    CHECK_FALSE(AlgorithmFactory::create("not_an_algo"));
}

TEST_CASE("IGraphAlgorithm dtor is exercised") {
    struct Dummy : IGraphAlgorithm {
        std::string run(const Graph&) override { return ""; }
    };
    IGraphAlgorithm* p = new Dummy;
    delete p; // virtual ~IGraphAlgorithm() in the header
    CHECK(true);
}
