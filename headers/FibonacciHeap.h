#pragma once

#include <limits>
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

class FibonacciHeap {
public:
    using Key = long long;
    struct Node {
        Key key;
        Node* parent = nullptr;
        Node* child = nullptr;
        Node* left;
        Node* right;
        int vertex;
        std::uint16_t degree = 0;
        bool mark = false;
        bool buffered = false;
        explicit Node(Key value = 0, int name = -1)
            : key(value), left(this), right(this), vertex(name) {}
    };

private:
    std::size_t size = 0;
    Node* min = nullptr;
    std::vector<Node*> degreeTable = std::vector<Node*>(2 * std::numeric_limits<std::size_t>::digits, nullptr);
    std::list<std::unique_ptr<Node[]>> buffers;
    Node* buffer = nullptr;
    std::size_t bufferCapacity = 0;
    std::size_t bufferUsed = 0;

public:
    FibonacciHeap() = default;
    explicit FibonacciHeap(std::size_t capacity) : bufferCapacity(capacity) {
        if (capacity != 0) {
            buffers.push_back(std::make_unique<Node[]>(capacity));
            buffer = buffers.back().get();
        }
    }
    FibonacciHeap(const FibonacciHeap&) = delete;
    FibonacciHeap& operator=(const FibonacciHeap&) = delete;

    ~FibonacciHeap() {
        while (min != nullptr) {
            Node* node = min;
            unionLists(node, node->child);
            node->left->right = node->right;
            node->right->left = node->left;
            min = node->right == node ? nullptr : node->right;
            if (!node->buffered) delete node;
        }
    }

    bool empty() const { return min == nullptr; }
    std::size_t getSize() const { return size; }

    void unionLists(Node* first, Node* second) {
        if (first == nullptr || second == nullptr) {
            return;
        }
        Node* left = first->left;
        Node* right = second->right;
        second->right = first;
        first->left = second;
        left->right = right;
        right->left = left;
    }

    Node* insert(Key key, int vertex = -1) {
        Node* node;
        if (bufferUsed < bufferCapacity) {
            node = &buffer[bufferUsed++];
            node->key = key;
            node->vertex = vertex;
            node->buffered = true;
        } else {
            node = new Node(key, vertex);
        }
        if (min == nullptr) {
            min = node;
        } else {
            Node* right = min->right;
            min->right = node;
            node->left = min;
            node->right = right;
            right->left = node;
            if (node->key < min->key) {
                min = node;
            }
        }
        ++size;
        return node;
    }

    int getMinVertex() const {
        if (empty()) {
            throw std::runtime_error("Heap is empty");
        }
        return min->vertex;
    }

    void merge(FibonacciHeap& other) {
        if (this == &other || other.empty()) {
            return;
        }
        if (empty()) {
            min = other.min;
            size = other.size;
        } else {
            unionLists(min, other.min);
            size += other.size;
            if (other.min->key < min->key) {
                min = other.min;
            }
        }
        other.min = nullptr;
        other.size = 0;
        buffers.splice(buffers.end(), other.buffers);
        if (bufferUsed == bufferCapacity) {
            buffer = other.buffer;
            bufferCapacity = other.bufferCapacity;
            bufferUsed = other.bufferUsed;
        }
        other.buffer = nullptr;
        other.bufferCapacity = 0;
        other.bufferUsed = 0;
    }

    void consolidate() {
        if (min == nullptr) {
            return;
        }
        Node* const first = min;
        Node* current = first;
        std::size_t highest = 0;
        do {
            Node* const next = current->right;
            if (current->parent != nullptr) {
                current->parent = nullptr;
                current->mark = false;
            }
            auto degree = current->degree;
            while (degreeTable[degree] != nullptr) {
                Node* conflict = degreeTable[degree];
                degreeTable[degree] = nullptr;
                if (conflict->key < current->key) {
                    std::swap(current, conflict);
                }
                conflict->parent = current;
                conflict->mark = false;
                if (current->child == nullptr) {
                    current->child = conflict;
                    conflict->left = conflict;
                    conflict->right = conflict;
                } else {
                    Node* child = current->child;
                    conflict->left = child->left;
                    conflict->right = child;
                    child->left->right = conflict;
                    child->left = conflict;
                }
                degree = ++current->degree;
            }
            degreeTable[degree] = current;
            if (degree > highest) {
                highest = degree;
            }
            current = next;
        } while (current != first);
        min = nullptr;
        for (std::size_t degree = 0; degree <= highest; ++degree) {
            Node* node = degreeTable[degree];
            degreeTable[degree] = nullptr;
            if (node == nullptr) {
                continue;
            }
            if (min == nullptr) {
                node->left = node;
                node->right = node;
                min = node;
            } else {
                node->left = min->left;
                node->right = min;
                min->left->right = node;
                min->left = node;
                if (node->key < min->key) {
                    min = node;
                }
            }
        }
    }

    Key deleteMin() {
        if (empty()) {
            throw std::runtime_error("Heap is empty");
        }
        Node* prevMin = min;
        const Key result = prevMin->key;
        unionLists(min, min->child);
        Node* left = min->left;
        Node* right = min->right;
        left->right = right;
        right->left = left;
        if (prevMin->right == prevMin) {
            min = nullptr;
        } else {
            min = right;
            consolidate();
        }
        --size;
        if (!prevMin->buffered) delete prevMin;
        return result;
    }

    void cut(Node* node) {
        Node* parent = node->parent;
        if (parent == nullptr) {
            return;
        }
        Node* left = node->left;
        Node* right = node->right;
        right->left = left;
        left->right = right;
        --parent->degree;
        if (parent->child == node) {
            parent->child = node->right == node ? nullptr : node->right;
        }
        node->parent = nullptr;
        node->mark = false;
        node->left = min->left;
        node->right = min;
        min->left->right = node;
        min->left = node;
    }

    void cascadingCut(Node* node) {
        while (node->parent != nullptr) {
            if (!node->mark) {
                node->mark = true;
                return;
            }
            Node* parent = node->parent;
            cut(node);
            node = parent;
        }
    }

    void decreaseKey(Node* node, Key newKey) {
        if (node == nullptr || newKey > node->key) {
            throw std::invalid_argument("Invalid node or increased key");
        }
        node->key = newKey;
        Node* parent = node->parent;
        if (parent != nullptr) {
            if (newKey >= parent->key) {
                return;
            }
            cut(node);
            cascadingCut(parent);
        }
        if (newKey < min->key) {
            min = node;
        }
    }

    void deleteNode(Node* node) {
        decreaseKey(node, INT64_MIN);
        if (node->parent != nullptr) {
            Node* parent = node->parent;
            cut(node);
            cascadingCut(parent);
        }
        min = node;
        deleteMin();
    }
};
