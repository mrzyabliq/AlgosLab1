#include "Dijkstra.h"
#include "FibonacciHeap.h"
#include "TwoHeap.h"
#include "TestGenerator.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class Action>
void requireThrows(Action action) {
    bool thrown = false;
    try {
        action();
    } catch (const std::exception&) {
        thrown = true;
    }
    require(thrown, "Expected an exception");
}

AdjacencyList emptyGraph(int n) {
    return AdjacencyList(static_cast<std::size_t>(n) + 1);
}

std::vector<long long> bellmanFord(const AdjacencyList& adjacency, int source) {
    const int n = static_cast<int>(adjacency.size()) - 1;
    std::vector<long long> distances(adjacency.size(), INT64_MAX);
    distances[source] = 0;
    for (int iteration = 1; iteration < n; ++iteration) {
        bool changed = false;
        for (int from = 1; from <= n; ++from) {
            if (distances[from] == INT64_MAX) {
                continue;
            }
            for (const auto& [to, weight] : adjacency[from]) {
                if (distances[from] < INT64_MAX - weight &&
                    distances[from] + weight < distances[to]) {
                    distances[to] = distances[from] + weight;
                    changed = true;
                }
            }
        }
        if (!changed) {
            break;
        }
    }
    return distances;
}

void checkPaths(const AdjacencyList& adjacency, int source, const DijkstraResult& result) {
    const int n = static_cast<int>(adjacency.size()) - 1;
    require(result.dist[source] == 0 && result.up[source] == 0, "Invalid source result");
    for (int vertex = 1; vertex <= n; ++vertex) {
        if (result.dist[vertex] == INT64_MAX) {
            require(result.up[vertex] == 0, "Unreachable vertex has a predecessor");
            continue;
        }
        int current = vertex;
        int steps = 0;
        while (current != source) {
            require(++steps <= n, "Predecessor cycle");
            const int parent = result.up[current];
            require(parent >= 1 && parent <= n, "Invalid predecessor");
            bool found = false;
            for (const auto& [to, weight] : adjacency[parent]) {
                if (to == current && result.dist[parent] < INT64_MAX - weight &&
                    result.dist[parent] + weight == result.dist[current]) {
                    found = true;
                    break;
                }
            }
            require(found, "Predecessor does not describe a shortest path");
            current = parent;
        }
    }
}

void checkGraph(const AdjacencyList& adjacency, int source) {
    const auto expected = bellmanFord(adjacency, source);
    const auto two = dijkstraTwoHeap(adjacency, source);
    const auto fibonacci = dijkstraFibonacciHeap(adjacency, source);
    require(two.dist == expected, "Binary-heap Dijkstra disagrees with Bellman-Ford");
    require(fibonacci.dist == expected, "Fibonacci-heap Dijkstra disagrees with Bellman-Ford");
    checkPaths(adjacency, source, two);
    checkPaths(adjacency, source, fibonacci);
}

void testDijkstra() {
    checkGraph(emptyGraph(1), 1);
    checkGraph(emptyGraph(8), 5);
    checkGraph(emptyGraph(10000), 5000);

    auto graph = emptyGraph(7);
    graph[1] = {{2, 100}, {2, 200}, {3, 1}};
    graph[3] = {{2, 2}};
    graph[2] = {{4, 0}};
    graph[4] = {{2, 0}, {5, 3000000000LL}};
    graph[5] = {{5, 0}};
    checkGraph(graph, 1);
    const auto result = dijkstraTwoHeap(graph, 1);
    require(result.dist[2] == 3 && result.dist[5] == 3000000003LL,
            "Repeated decrease or 64-bit distance failed");
    checkGraph(graph, 5);

    auto nearLimit = emptyGraph(5);
    nearLimit[1] = {{2, INT64_MAX - 2}, {4, 20}};
    nearLimit[2] = {{3, 1}};
    nearLimit[3] = {{4, 10}, {5, 1}};
    checkGraph(nearLimit, 1);
    const auto large = dijkstraFibonacciHeap(nearLimit, 1);
    require(large.dist[3] == INT64_MAX - 1 && large.dist[4] == 20 && large.dist[5] == INT64_MAX,
            "Overflow handling failed");

    AdjacencyList undirected{{}, {{2, 0}}, {{1, 0}, {3, 4}}, {{2, 4}}};
    checkGraph(undirected, 3);

    auto chain = emptyGraph(1000);
    for (int i = 1; i < 1000; ++i) {
        chain[i].push_back({i + 1, 1});
    }
    checkGraph(chain, 1);
    require(dijkstraFibonacciHeap(chain, 1).dist[1000] == 999, "Sparse chain failed");

    std::mt19937 random(1729);
    for (int trial = 0; trial < 500; ++trial) {
        const int n = 1 + static_cast<int>(random() % 40);
        auto generated = emptyGraph(n);
        const int edges = static_cast<int>(random() % (n * n + 1));
        for (int i = 0; i < edges; ++i) {
            const int from = 1 + static_cast<int>(random() % n);
            const int to = 1 + static_cast<int>(random() % n);
            generated[from].push_back({to, random() % 100});
        }
        checkGraph(generated, 1 + static_cast<int>(random() % n));
    }
}

void testGenerator() {
    for (int n : {1, 2, 7, 50}) {
        const auto capacity = static_cast<std::size_t>(n) * static_cast<std::size_t>(n - 1);
        for (std::size_t m : {static_cast<std::size_t>(n - 1), capacity / 2, capacity}) {
            TestGenerator first(123), second(123);
            const auto adjacency = first.generate(n, m, 0, 9);
            require(adjacency == second.generate(n, m, 0, 9), "Seed is not reproducible");
            require(adjacency.size() == static_cast<std::size_t>(n) + 1 && adjacency[0].empty(),
                    "Wrong generated vertex count");
            std::set<std::pair<int, int>> edges;
            for (int from = 1; from <= n; ++from) {
                for (const auto& [to, weight] : adjacency[from]) {
                    require(to >= 1 && to <= n && to != from, "Invalid generated edge");
                    require(weight >= 0 && weight <= 9, "Invalid generated weight");
                    require(edges.insert({from, to}).second, "Generator produced duplicate edges");
                }
            }
            require(edges.size() == m, "Wrong generated edge count");
            const auto result = dijkstraTwoHeap(adjacency, 1);
            for (int vertex = 1; vertex <= n; ++vertex) {
                require(result.dist[vertex] != INT64_MAX, "Generated vertex is unreachable from 1");
            }
            checkGraph(adjacency, 1);
            checkGraph(adjacency, n);
        }
    }
    TestGenerator generator(42);
    const auto zero = generator.generate(5, 20, 0, 0);
    for (const auto& row : zero) {
        for (const auto& edge : row) require(edge.second == 0, "Constant weights changed");
    }
    checkGraph(zero, 1);
    checkGraph(generator.generate(4, 12, INT64_MAX - 10, INT64_MAX - 1), 2);

    for (std::uint64_t seed = 0; seed < 30; ++seed) {
        TestGenerator trees(seed);
        const auto result = dijkstraTwoHeap(trees.generate(100, 99, 1, 100), 1);
        for (int vertex = 1; vertex <= 100; ++vertex) {
            require(result.dist[vertex] != INT64_MAX, "Generated tree is unreachable from 1");
        }
    }

    for (int n : {1, 2, 3, 6, 40}) {
        const auto graph = generator.generateManyDecreases(n);
        const int chain = n / 2, targets = n - chain;
        std::set<std::pair<int, int>> edges;
        for (int from = 1; from <= n; ++from) {
            for (const auto& [to, weight] : graph[from]) {
                require(to > from && to <= n && weight > 0, "Invalid stress-test edge");
                require(edges.insert({from, to}).second, "Duplicate stress-test edge");
            }
        }
        const std::size_t expectedEdges = chain == 0 ? 0 : std::size_t(chain) * targets + chain - 1;
        require(edges.size() == expectedEdges, "Wrong stress-test edge count");
        checkGraph(graph, 1);
        const auto result = dijkstraFibonacciHeap(graph, 1);
        for (int vertex = 1; vertex <= n; ++vertex) {
            const long long expected = vertex == 1 ? 0 : vertex <= chain ? vertex - 1 : targets + 1LL + vertex;
            require(result.dist[vertex] == expected, "Wrong stress-test shortest distance");
        }
    }
}

void checkFibonacciStructure(const FibonacciHeap& heap, const FibonacciHeap::Node* anchor) {
    if (heap.empty()) {
        require(heap.getSize() == 0, "Empty heap has a nonzero size");
        return;
    }
    std::set<const FibonacciHeap::Node*> visited;
    std::function<void(const FibonacciHeap::Node*, const FibonacciHeap::Node*)> visit;
    visit = [&](const FibonacciHeap::Node* first, const FibonacciHeap::Node* parent) {
        const auto* node = first;
        do {
            require(visited.insert(node).second, "Node occurs twice in Fibonacci heap");
            require(node->left->right == node && node->right->left == node,
                    "Broken circular list");
            require(node->parent == parent, "Wrong parent pointer");
            require(parent == nullptr || parent->key <= node->key, "Broken heap order");
            std::size_t children = 0;
            if (node->child != nullptr) {
                const auto* child = node->child;
                do {
                    require(++children <= heap.getSize(), "Child list does not close");
                    child = child->right;
                } while (child != node->child);
                visit(node->child, node);
            }
            require(children == node->degree, "Wrong node degree");
            node = node->right;
        } while (node != first);
    };
    require(anchor != nullptr, "Nonempty heap has no live anchor");
    std::size_t depth = 0;
    while (anchor->parent != nullptr) {
        require(++depth <= heap.getSize(), "Parent cycle");
        anchor = anchor->parent;
    }
    visit(anchor, nullptr);
    require(visited.size() == heap.getSize(), "Heap lost nodes");
    long long smallest = INT64_MAX;
    long long selected = INT64_MAX;
    bool found = false;
    for (const auto* node : visited) {
        smallest = std::min(smallest, node->key);
        if (node->parent == nullptr && node->vertex == heap.getMinVertex()) {
            selected = node->key;
            found = true;
        }
    }
    require(found && selected == smallest, "Incorrect minimum root");
}

FibonacciHeap::Node* anyNode(const std::vector<FibonacciHeap::Node*>& nodes) {
    for (auto* node : nodes) {
        if (node != nullptr) {
            return node;
        }
    }
    return nullptr;
}

void testHeapOperations() {
    TwoHeap reserved;
    requireThrows([&] { reserved.indexOf(1); });
    require(reserved.getParentIndex(0) == INT64_MAX, "Root has a parent");
    require(reserved.getLeftChildIndex(0) == INT64_MAX, "Empty heap has a child");
    require(reserved.getRightChildIndex(0) == INT64_MAX, "Empty heap has a last child");
    reserved.insert(42, 1);
    requireThrows([&] { reserved.insert(43, 1); });
    require(reserved.getLeftChildIndex(0) == INT64_MAX, "Single node has a child");
    reserved.insert(7, 2);
    require(reserved.getRightChildIndex(0) == 1, "Last child must be the only existing child");
    reserved.extractMin();
    requireThrows([&] { reserved.indexOf(2); });
    require(reserved.getMin() == 42, "Wrong minimum after extraction");
    reserved.decreaseWeight(reserved.indexOf(1), 20);
    require(reserved.getMin() == 22, "DecreaseWeight must subtract its argument");
    reserved.extractMin();
    requireThrows([&] { reserved.indexOf(1); });

    for (int heapType : {0, 1, 2}) {
        const bool useFibonacci = heapType != 0;
        TwoHeap binary;
        FibonacciHeap fibonacci(heapType == 2 ? 128 : 0);
        std::vector<FibonacciHeap::Node*> nodes(128, nullptr);
        std::vector<bool> active(128, false);
        std::vector<long long> keys(128);
        std::set<std::pair<long long, int>> expected;
        std::mt19937 random(42);
        for (int step = 0; step < 6000; ++step) {
            const int vertex = static_cast<int>(random() % active.size());
            const int operation = static_cast<int>(random() % 4);
            if (!active[vertex]) {
                keys[vertex] = random() % 1000;
                if (useFibonacci) {
                    nodes[vertex] = fibonacci.insert(keys[vertex], vertex);
                } else {
                    binary.insert(keys[vertex], vertex);
                }
                active[vertex] = true;
                expected.insert({keys[vertex], vertex});
            } else if (operation == 0) {
                expected.erase({keys[vertex], vertex});
                const long long decrease = random() % 100;
                keys[vertex] -= decrease;
                if (useFibonacci) {
                    fibonacci.decreaseKey(nodes[vertex], keys[vertex]);
                } else {
                    binary.decreaseWeight(binary.indexOf(vertex), decrease);
                }
                expected.insert({keys[vertex], vertex});
            } else if (operation == 1) {
                if (useFibonacci) {
                    fibonacci.deleteNode(nodes[vertex]);
                } else {
                    binary.deleteIndex(binary.indexOf(vertex));
                }
                nodes[vertex] = nullptr;
                active[vertex] = false;
                expected.erase({keys[vertex], vertex});
            } else {
                const int removed = useFibonacci ? fibonacci.getMinVertex() : binary.getMinVertex();
                require(!useFibonacci || nodes.at(removed) != nullptr, "Unknown minimum vertex");
                const long long key = useFibonacci ? nodes[removed]->key : binary.getMin();
                require(key == expected.begin()->first && expected.count({key, removed}) == 1,
                        "Incorrect heap minimum");
                if (useFibonacci) {
                    require(fibonacci.deleteMin() == key, "Wrong extracted key");
                } else {
                    binary.extractMin();
                }
                nodes[removed] = nullptr;
                active[removed] = false;
                expected.erase({key, removed});
            }
            require((useFibonacci ? fibonacci.getSize() : binary.getSize()) == expected.size(),
                    "Wrong heap size");
            if (useFibonacci) {
                checkFibonacciStructure(fibonacci, anyNode(nodes));
            } else {
                for (int id = 0; id < static_cast<int>(active.size()); ++id) {
                    if (active[id]) {
                        require(binary.indexOf(id) < static_cast<std::int64_t>(binary.getSize()),
                                "Invalid binary vertex index");
                    }
                }
            }
        }
    }

    TwoHeap binary;
    binary.buildHeap({7, 1, 8, -5, 0, -5});
    for (long long key : {-5, -5, 0, 1, 7, 8}) {
        require(binary.getMin() == key, "Build heap or sift down failed");
        binary.extractMin();
    }
    requireThrows([&] { binary.extractMin(); });
    binary.buildHeap({});
    require(binary.empty(), "Building an empty heap failed");
    binary.insert(INT64_MIN, 1);
    binary.insert(INT64_MIN, 2);
    binary.deleteIndex(binary.indexOf(2));
    require(binary.getSize() == 1 && binary.getMinVertex() == 1,
            "Deleting a specific tied minimum failed");
    requireThrows([&] { binary.decreaseWeight(binary.indexOf(1), 1); });
    requireThrows([&] { binary.decreaseWeight(binary.indexOf(1), -1); });

    TwoHeap built;
    built.buildHeap({8, INT64_MAX, 3, 3000000000LL});
    require(built.getMin() == 3 && built.getMinVertex() == 3,
            "Heapify changed vertex names");
    built.decreaseWeight(built.indexOf(2), INT64_MAX - 1);
    require(built.getMin() == 1 && built.getMinVertex() == 2,
            "Heapify did not initialize reverse indices or 64-bit keys");
    built.extractMin();
    requireThrows([&] { built.indexOf(2); });
    built.buildHeap({5});
    require(built.getSize() == 1 && built.getMinVertex() == 1 && built.getMin() == 5,
            "Rebuilding a populated heap failed");
    requireThrows([&] { built.indexOf(4); });
    built.buildHeap({});
    require(built.empty(), "Rebuilding with an empty array failed");
    requireThrows([&] { built.indexOf(1); });

    FibonacciHeap empty;
    requireThrows([&] { empty.deleteMin(); });
    auto* first = empty.insert(INT64_MIN, 1);
    auto* second = empty.insert(INT64_MIN, 2);
    empty.deleteNode(first);
    require(empty.getMinVertex() == 2, "Deleting a node with a tied minimum failed");
    requireThrows([&] { empty.decreaseKey(second, 10); });
    FibonacciHeap other;
    other.insert(-10, 3);
    empty.merge(other);
    require(other.empty() && empty.getSize() == 2, "Merge did not transfer ownership");
    empty.merge(empty);
    checkFibonacciStructure(empty, second);
}

void testFibonacciBuffers() {
    FibonacciHeap destination(2);
    auto* first = destination.insert(9, 1);
    auto* second = destination.insert(7, 2);
    require(second == first + 1, "Reserved nodes are not contiguous");
    FibonacciHeap::Node* transferred = nullptr;
    {
        FibonacciHeap source(3);
        transferred = source.insert(5, 3);
        source.insert(11, 4);
        source.insert(3, 5);
        destination.merge(source);
        source.insert(100, 99);
        require(source.getSize() == 1, "Merged source cannot be reused");
    }
    destination.insert(4, 6);
    destination.consolidate();
    destination.decreaseKey(transferred, -8);
    checkFibonacciStructure(destination, first);
    for (long long key : {-8, 3, 4, 7, 9, 11}) {
        require(destination.deleteMin() == key, "Buffered merge lost a node");
    }
    destination.insert(2, 7);
    require(destination.deleteMin() == 2 && destination.empty(), "Exhausted buffer cannot be reused");

    FibonacciHeap adopted;
    FibonacciHeap::Node* start = nullptr;
    {
        FibonacciHeap source(4);
        start = source.insert(10, 1);
        source.insert(20, 2);
        adopted.merge(source);
    }
    require(adopted.insert(30, 3) == start + 2, "Unused transferred buffer slots were lost");
    checkFibonacciStructure(adopted, start);

    FibonacciHeap reservedDestination(4);
    {
        FibonacciHeap source(2);
        source.insert(1, 1);
        reservedDestination.merge(source);
    }
    reservedDestination.insert(0, 2);
    require(reservedDestination.deleteMin() == 0 && reservedDestination.deleteMin() == 1,
            "Merge into an empty reserved heap failed");
}

void testCascadingCuts() {
    FibonacciHeap heap;
    std::vector<FibonacciHeap::Node*> nodes;
    for (int i = 0; i <= 64; ++i) {
        nodes.push_back(heap.insert(i, i));
    }
    heap.deleteMin();
    FibonacciHeap::Node* parent = nullptr;
    for (std::size_t i = 1; i < nodes.size(); ++i) {
        if (nodes[i]->parent != nullptr && nodes[i]->degree >= 2) {
            parent = nodes[i];
            break;
        }
    }
    require(parent != nullptr, "No node suitable for cascading cut test");
    auto* first = parent->child;
    auto* second = first->right;
    heap.decreaseKey(first, -1000);
    require(parent->mark && parent->parent != nullptr, "First child loss should mark parent");
    heap.decreaseKey(second, -2000);
    require(parent->parent == nullptr && !parent->mark, "Second child loss should cut parent");
    checkFibonacciStructure(heap, nodes[1]);
}

void testConsolidationTraversal() {
    for (int mode = 0; mode < 3; ++mode) {
        FibonacciHeap heap;
        std::vector<FibonacciHeap::Node*> nodes;
        std::set<std::pair<long long, int>> expected;
        int group = 0;
        for (int degree : {0, 2, 0, 1, 3, 1, 0, 2, 4, 0}) {
            const int count = 1 << degree;
            FibonacciHeap part(static_cast<std::size_t>(count) + 1);
            part.insert(INT64_MIN, -1);
            const long long key = mode == 0 ? 100 : mode == 1 ? 100 - group : 100 + group;
            for (int i = 0; i < count; ++i) {
                const int vertex = static_cast<int>(nodes.size());
                nodes.push_back(part.insert(key, vertex));
                expected.insert({key, vertex});
            }
            part.deleteMin();
            heap.merge(part);
            ++group;
        }
        for (int repeat = 0; repeat < 3; ++repeat) {
            heap.consolidate();
            checkFibonacciStructure(heap, anyNode(nodes));
        }
        while (!expected.empty()) {
            const int vertex = heap.getMinVertex();
            const long long key = heap.deleteMin();
            require(key == expected.begin()->first && expected.erase({key, vertex}) == 1,
                    "Consolidation skipped or duplicated a root");
            nodes[vertex] = nullptr;
            checkFibonacciStructure(heap, anyNode(nodes));
        }
        require(heap.empty(), "Consolidation retained extra nodes");
    }
}

void testFibonacciMerge() {
    for (int firstCount : {0, 1, 2, 33}) {
        for (int secondCount : {0, 1, 2, 65}) {
            FibonacciHeap first, second;
            std::vector<FibonacciHeap::Node*> nodes(501, nullptr);
            std::set<std::pair<long long, int>> expected;
            for (int i = 0; i < firstCount; ++i) {
                nodes[i] = first.insert((i * 37) % 101, i);
                expected.insert({(i * 37) % 101, i});
            }
            for (int i = 0; i < secondCount; ++i) {
                nodes[100 + i] = second.insert((i * 11) % 47, 100 + i);
                expected.insert({(i * 11) % 47, 100 + i});
            }
            for (auto* heap : {&first, &second}) {
                if (heap->getSize() > 2) {
                    const int vertex = heap->getMinVertex();
                    const long long key = heap->deleteMin();
                    nodes[vertex] = nullptr;
                    expected.erase({key, vertex});
                }
            }
            first.merge(second);
            require(second.empty(), "Merged heap retained its nodes");
            checkFibonacciStructure(first, anyNode(nodes));
            auto* transferred = second.insert(5000, 500);
            nodes[500] = transferred;
            first.merge(second);
            first.decreaseKey(transferred, -1000);
            expected.insert({-1000, 500});
            second.insert(-300, 999);
            first.consolidate();
            first.consolidate();
            checkFibonacciStructure(first, anyNode(nodes));
            while (!expected.empty()) {
                const int vertex = first.getMinVertex();
                const long long key = nodes.at(vertex)->key;
                require(key == expected.begin()->first && expected.count({key, vertex}) == 1,
                        "Merge lost a key or vertex");
                first.deleteMin();
                nodes[vertex] = nullptr;
                expected.erase({key, vertex});
                checkFibonacciStructure(first, anyNode(nodes));
            }
            require(first.empty() && second.getSize() == 1 && second.getMinVertex() == 999,
                    "Merge did not separate ownership");
        }
    }
}
}

int main() {
    try {
        testHeapOperations();
        testFibonacciBuffers();
        testCascadingCuts();
        testConsolidationTraversal();
        testFibonacciMerge();
        testDijkstra();
        testGenerator();
        std::cout << "Passed: heap models (6000 operations each), cascading cuts, edge cases, "
                     "500 random graphs against Bellman-Ford.\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
