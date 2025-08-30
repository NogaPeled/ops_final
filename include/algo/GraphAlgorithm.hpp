#pragma once
#include "graph/Graph.hpp"      // your project Graph
#include <memory>
#include <string>

// Strategy interface all algorithms implement
struct IGraphAlgorithm {
    virtual ~IGraphAlgorithm() = default;
    virtual std::string run(const Graph& g) = 0;
};

// Factory that returns a concrete strategy by name
// Accepts: "MST", "SCC", "MAXFLOW", "HAMILTON" (case-insensitive)
struct AlgorithmFactory {
    static std::unique_ptr<IGraphAlgorithm> create(const std::string& name);
};
