#pragma once

#include "Dijkstra.h"

#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <unordered_set>

class TestGenerator {
    std::mt19937_64 random;

public:
    explicit TestGenerator(std::uint64_t seed = 42) : random(seed) {}

    AdjacencyList generateManyDecreases(int n) {
        const int chain = n / 2;
        const int targets = n - chain;
        AdjacencyList adjacency(static_cast<std::size_t>(n) + 1);
        for (int from = 1; from <= chain; ++from) {
            adjacency[from].reserve(targets + 1);
            if (from < chain) adjacency[from].push_back({from + 1, 1});
            const long long base = (chain - from + 1LL) * (targets + 1LL) + chain;
            for (int target = targets; target >= 1; --target) {
                adjacency[from].push_back({chain + target, base + target - (from - 1)});
            }
        }
        return adjacency;
    }

    AdjacencyList generate(int n, std::size_t m, long long minWeight, long long maxWeight) {
        const auto maxEdges = static_cast<std::uint64_t>(n) * static_cast<std::uint64_t>(n - 1);
        AdjacencyList adjacency(static_cast<std::size_t>(n) + 1);
        std::unordered_set<std::uint64_t> selected;
        selected.reserve(m);
        std::uniform_int_distribution<long long> weight(minWeight, maxWeight);
        
        for (int to = 2; to <= n; ++to) {
            std::uniform_int_distribution<int> parent(1, to - 1);
            const int from = parent(random);
            const auto edge = static_cast<std::uint64_t>(from - 1) * (n - 1) + to - 2;
            selected.insert(edge);
            adjacency[from].push_back({to, weight(random)});
        }

        while (selected.size() < m) {
            std::uniform_int_distribution<std::uint64_t> pick(0, maxEdges - 1);
            const auto edge = pick(random);
            if (!selected.insert(edge).second) continue;
            const int from = static_cast<int>(edge / static_cast<std::uint64_t>(n - 1)) + 1;
            int to = static_cast<int>(edge % static_cast<std::uint64_t>(n - 1)) + 1;
            if (to >= from) {
                ++to;
            }
            adjacency[from].push_back({to, weight(random)});
        }
        return adjacency;
    }
};
