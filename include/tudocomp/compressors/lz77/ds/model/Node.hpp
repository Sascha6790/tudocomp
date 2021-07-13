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
}