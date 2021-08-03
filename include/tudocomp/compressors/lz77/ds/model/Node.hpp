#pragma once

#include <cmath>
#include <concepts>

namespace tdc {
    template<typename T> requires std::integral<T>
    struct Node {
        Node *parent;
        const char *edgeLabel; // (parent, this)
        T edgeLabelLength;
        T nodeLabel;
        uint depth;

        Node *rightmost = nullptr;
        std::unordered_map<char, Node *> childNodes;

        explicit Node(Node *parent) : parent(parent),
                                      edgeLabel(nullptr),
                                      edgeLabelLength(1),
                                      nodeLabel(0),
                                      depth(0) {}
    };

    template<typename T> requires std::integral<T>
    struct WeightedNode {
        WeightedNode *parent;
        const char *edgeLabel = nullptr; // (parent, this)
        T edgeLabelLength = 1;
        T nodeLabel = 0;
        uint depth = 0;

        WeightedNode *rightmost = nullptr;
        std::map<char, WeightedNode *> childNodes;
        T minLabel = std::numeric_limits<T>::max();
        T maxLabel = 0;

        explicit    WeightedNode(WeightedNode *parent) : parent(parent) {}
    };
}