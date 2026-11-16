#pragma once

#include "Visualizer.h"
#include "TestGenerator.h"

#include <chrono>
#include <iostream>
#include <istream>
#include <limits>
#include <stdexcept>
#include <string>

inline void printHelp() {
    std::cout <<
        "Usage:\n"
        "  dijkstra --help                  Show this help (-h also works)\n"
        "  dijkstra --benchmark             Run benchmark (also the default with no arguments)\n"
        "  dijkstra matrix-file             Read an adjacency matrix from a file\n"
        "  dijkstra -                       Enter an adjacency matrix from the keyboard\n"
        "  dijkstra --random                Enter n and m from the keyboard (source = 1)\n"
        "  dijkstra --random n m [source]   Generate a random graph; default source = 1\n\n"
        "Matrix format: n source, followed by n*n weights.\n"
        "-1 means no edge; other weights are nonnegative. Zero diagonal entries are ignored.\n\n"
        "Random graph: directed, no loops or duplicate edges, weights 1..100.\n"
        "n >= 1, n-1 <= m <= n*(n-1), 1 <= source <= n. All vertices are reachable.\n\n"
        "Graph modes write graph.html with shortest paths; benchmark writes benchmark.html.\n"
        "Drawing is limited to 80 vertices and 250 edges; distances include the whole graph.\n\n"
        "Examples:\n"
        "  dijkstra examples/graph.txt\n"
        "  dijkstra --random 8 16\n"
        "  dijkstra --random 8 16 3\n";
}

inline long long parseNumber(const std::string& text) {
    std::size_t end;
    const long long value = std::stoll(text, &end);
    if (end != text.size()) throw std::runtime_error("Invalid number: " + text);
    return value;
}

inline AdjacencyList readMatrix(std::istream& input, int& source) {
    int n;
    if (!(input >> n >> source) || n < 1 || source < 1 || source > n) {
        throw std::runtime_error("Expected n and source (1 <= source <= n)");
    }
    AdjacencyList adjacency(static_cast<std::size_t>(n) + 1);
    for (int from = 1; from <= n; ++from) {
        for (int to = 1; to <= n; ++to) {
            long long weight;
            if (!(input >> weight) || weight < -1) {
                throw std::runtime_error("Expected n*n weights: -1 for no edge, otherwise nonnegative");
            }
            if (weight != -1 && !(from == to && weight == 0)) {
                adjacency[from].push_back({to, weight});
            }
        }
    }
    return adjacency;
}

inline void runGraph(const AdjacencyList& adjacency, int source) {
    const auto startTwo = std::chrono::steady_clock::now();
    const auto two = dijkstraTwoHeap(adjacency, source);
    const double timeTwo = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - startTwo).count();
    const auto startFibonacci = std::chrono::steady_clock::now();
    const auto fibonacci = dijkstraFibonacciHeap(adjacency, source);
    const double timeFibonacci = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - startFibonacci).count();
    if (two.dist != fibonacci.dist) {
        throw std::runtime_error("Algorithms produced different distances");
    }
    for (std::size_t vertex = 1; vertex < two.dist.size(); ++vertex) {
        std::cout << vertex << ": ";
        if (two.dist[vertex] == INT64_MAX) std::cout << "INF";
        else std::cout << two.dist[vertex];
        std::cout << '\n';
    }
    Visualizer{}.saveHtml("graph.html", adjacency, source, two, timeTwo, fibonacci, timeFibonacci);
    std::cout << "Graph and shortest paths: graph.html\n";
}

inline void runGraph(std::istream& input) {
    int source;
    const auto adjacency = readMatrix(input, source);
    runGraph(adjacency, source);
}

inline void runRandomGraph(int argc, char* argv[]) {
    long long vertexCount, edgeCount, source = 1;
    if (argc == 2) {
        std::cout << "Enter n and m (source = 1):\n";
        if (!(std::cin >> vertexCount >> edgeCount)) {
            throw std::runtime_error("Expected the number of vertices and edges");
        }
    } else if (argc == 4 || argc == 5) {
        vertexCount = parseNumber(argv[2]);
        edgeCount = parseNumber(argv[3]);
        if (argc == 5) source = parseNumber(argv[4]);
    } else {
        throw std::runtime_error("Usage: dijkstra --random [n m [source]]");
    }
    if (vertexCount < 1 || vertexCount > std::numeric_limits<int>::max()) {
        throw std::runtime_error("n must be a positive int");
    }
    if (edgeCount < vertexCount - 1 || edgeCount > vertexCount * (vertexCount - 1)) {
        throw std::runtime_error("m must satisfy n-1 <= m <= n*(n-1)");
    }
    if (source < 1 || source > vertexCount) {
        throw std::runtime_error("Source must be between 1 and n");
    }
    TestGenerator generator(std::random_device{}());
    auto adjacency = generator.generate(static_cast<int>(vertexCount), static_cast<std::size_t>(edgeCount), 1, 100);
    if (source != 1) {
        std::swap(adjacency[1], adjacency[static_cast<std::size_t>(source)]);
        for (auto& row : adjacency) {
            for (auto& edge : row) {
                if (edge.first == 1) edge.first = static_cast<int>(source);
                else if (edge.first == source) edge.first = 1;
            }
        }
    }
    std::cout << "Random graph: n=" << vertexCount << ", m=" << edgeCount << ", source=" << source << '\n';
    runGraph(adjacency, static_cast<int>(source));
}
