#pragma once

#include "FibonacciHeap.h"
#include "TwoHeap.h"

#include <cstdint>
#include <utility>
#include <vector>

using AdjacencyList = std::vector<std::vector<std::pair<int, long long>>>;

struct DijkstraResult {
    std::vector<long long> dist;
    std::vector<int> up;
};

inline DijkstraResult dijkstraTwoHeap(const AdjacencyList& adjacency, int source) {
    DijkstraResult result{std::vector<long long>(adjacency.size(), INT64_MAX),
                          std::vector<int>(adjacency.size(), 0)};
    result.dist[source] = 0;
    TwoHeap queue;
    queue.insert(0, source);

    while (!queue.empty()) {
        const int current = queue.getMinVertex();
        const long long distance = queue.getMin();
        queue.extractMin();
        for (const auto& [next, weight] : adjacency[current]) {
            if (distance >= INT64_MAX - weight) {
                continue;
            }
            const long long candidate = distance + weight;
            if (candidate < result.dist[next]) {
                if (result.dist[next] == INT64_MAX) {
                    queue.insert(candidate, next);
                } else {
                    queue.decreaseWeight(queue.indexOf(next), result.dist[next] - candidate);
                }
                result.dist[next] = candidate;
                result.up[next] = current;
            }
        }
    }
    return result;
}

inline DijkstraResult dijkstraFibonacciHeap(const AdjacencyList& adjacency, int source) {
    const int n = static_cast<int>(adjacency.size()) - 1;
    DijkstraResult result{std::vector<long long>(adjacency.size(), INT64_MAX),
                          std::vector<int>(adjacency.size(), 0)};
    result.dist[source] = 0;
    FibonacciHeap queue(static_cast<std::size_t>(n));
    std::vector<FibonacciHeap::Node*> nodes(adjacency.size(), nullptr);
    nodes[source] = queue.insert(0, source);

    while (!queue.empty()) {
        const int current = queue.getMinVertex();
        const long long distance = queue.deleteMin();
        nodes[current] = nullptr;
        for (const auto& [next, weight] : adjacency[current]) {
            if (distance >= INT64_MAX - weight) {
                continue;
            }
            const long long candidate = distance + weight;
            if (candidate < result.dist[next]) {
                result.dist[next] = candidate;
                result.up[next] = current;
                if (nodes[next] == nullptr) {
                    nodes[next] = queue.insert(candidate, next);
                } else {
                    queue.decreaseKey(nodes[next], candidate);
                }
            }
        }
    }
    return result;
}
