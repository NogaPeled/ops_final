#pragma once                              // ensure this header is included only once per translation unit
#include "graph/Graph.hpp"                // include our Graph class definition so we can reference it
#include <string>                         // include std::string since run() returns a human-readable message

/**
 * @brief Euler circuit finder for undirected graphs using Hierholzer's algorithm.  // high-level description
 *        It verifies existence conditions and constructs the circuit if possible.   // explain purpose
 */
class Euler {                             // begin Euler class declaration
public:                                   // public API section
    // Execute the Euler-circuit routine on the given graph and return a message.   // describe method responsibility
    std::string run(const Graph& g);      // declaration of the main algorithm entry point
};                                        // end of Euler class
