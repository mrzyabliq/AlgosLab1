#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

class TwoHeap {
public:
    using Key = long long;

private:
    std::vector<Key> weights;
    std::vector<int> names;
    std::vector<std::int64_t> indices;

    void swapNodes(std::int64_t first, std::int64_t second) {
        std::swap(weights[first], weights[second]);
        std::swap(names[first], names[second]);
        indices[names[first]] = first;
        indices[names[second]] = second;
    }

    std::int64_t getMinChildIndex(std::int64_t position) const {
        const auto left = getLeftChildIndex(position);
        if (left == INT64_MAX) {
            return INT64_MAX;
        }
        const auto right = getRightChildIndex(position);
        return weights[left] < weights[right] ? left : right;
    }

public:
    TwoHeap() = default;

    bool empty() const { return weights.empty(); }
    std::size_t getSize() const { return weights.size(); }

    std::int64_t indexOf(int vertex) const {
        if (vertex < 0 || static_cast<std::size_t>(vertex) >= indices.size() ||
            indices[vertex] == INT64_MAX) {
            throw std::out_of_range("Vertex is not in the heap");
        }
        return indices[vertex];
    }

    std::int64_t getParentIndex(std::int64_t childIndex) const {
        return childIndex == 0 ? INT64_MAX : (childIndex - 1) / 2;
    }

    std::int64_t getLeftChildIndex(std::int64_t parentIndex) const {
        const auto n = static_cast<std::int64_t>(weights.size());
        if (parentIndex < 0 || n < 2 || parentIndex > (n - 2) / 2) {
            return INT64_MAX;
        }
        return 2 * parentIndex + 1;
    }

    std::int64_t getRightChildIndex(std::int64_t parentIndex) const {
        const auto left = getLeftChildIndex(parentIndex);
        return left == INT64_MAX ? INT64_MAX :
            std::min(left + 1, static_cast<std::int64_t>(weights.size()) - 1);
    }

    void diving(std::int64_t position) {
        auto current = position;
        auto child = getMinChildIndex(current);
        while (child != INT64_MAX && weights[current] > weights[child]) {
            swapNodes(current, child);
            current = child;
            child = getMinChildIndex(current);
        }
    }

    void emersion(std::int64_t position) {
        while (position > 0 && weights[position] < weights[getParentIndex(position)]) {
            const auto parent = getParentIndex(position);
            swapNodes(position, parent);
            position = parent;
        }
    }

    void insert(Key key, int vertex = -1) {
        if (vertex == -1) {
            vertex = static_cast<int>(indices.size());
        }
        if (vertex < 0) {
            throw std::invalid_argument("Invalid vertex");
        }
        if (static_cast<std::size_t>(vertex) >= indices.size()) {
            indices.resize(static_cast<std::size_t>(vertex) + 1, INT64_MAX);
        }
        if (indices[vertex] != INT64_MAX) {
            throw std::invalid_argument("Duplicate vertex");
        }
        weights.push_back(key);
        names.push_back(vertex);
        indices[vertex] = static_cast<std::int64_t>(weights.size()) - 1;
        emersion(indices[vertex]);
    }

    Key getMin() const {
        if (empty()) {
            throw std::runtime_error("Heap is empty");
        }
        return weights.front();
    }

    int getMinVertex() const {
        if (empty()) {
            throw std::runtime_error("Heap is empty");
        }
        return names.front();
    }

    void decreaseWeight(std::int64_t position, Key decrease) {
        if (position < 0 || static_cast<std::size_t>(position) >= weights.size() ||
            decrease < 0) {
            throw std::invalid_argument("Invalid index or negative decrease");
        }
        if (weights[position] < INT64_MIN + decrease) {
            throw std::overflow_error("Key underflow");
        }
        weights[position] -= decrease;
        emersion(position);
    }

    void extractMin() {
        if (empty()) {
            throw std::runtime_error("Heap is empty");
        }
        const int removed = names.front();
        swapNodes(0, static_cast<std::int64_t>(weights.size()) - 1);
        weights.pop_back();
        names.pop_back();
        indices[removed] = INT64_MAX;
        if (!empty()) {
            diving(0);
        }
    }

    void deleteIndex(std::int64_t position) {
        if (position < 0 || static_cast<std::size_t>(position) >= weights.size()) {
            throw std::out_of_range("Index out of range");
        }
        weights[position] = INT64_MIN;
        while (position > 0) {
            const auto parent = getParentIndex(position);
            swapNodes(position, parent);
            position = parent;
        }
        extractMin();
    }

    void buildHeap(const std::vector<Key>& inputWeights) {
        weights = inputWeights;
        names.resize(weights.size());
        indices.assign(weights.size() + 1, INT64_MAX);
        for (std::size_t i = 0; i < weights.size(); ++i) {
            names[i] = static_cast<int>(i) + 1;
            indices[names[i]] = static_cast<std::int64_t>(i);
        }
        for (auto i = static_cast<std::int64_t>(weights.size()) - 1; i >= 0; --i) {
            diving(i);
        }
    }
};
