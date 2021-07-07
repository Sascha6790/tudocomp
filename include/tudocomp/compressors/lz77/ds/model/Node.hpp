#pragma once

#include <cmath>

/**
 * Nodes are classified in NodeType::HEAVY and NodeType::LIGHT
 * Nodes are of type HEAVY if they have at least \Theta(lg^2 lg \sigma) =: heavyThreshold leaves.
 */
namespace tdc {
    enum NodeType {
        HEAVY,
        LIGHT
    };

    // Branching Heavy Node: Node is heavy and has more than one heavy child
    // -> Store Heavy children in dictionary[firstCharOfChild]
    // The dictionary has at most \signma entries.
    //
    struct Node {
        Node *parent;
        char *text;
        size_t length;
        NodeType nodeType;
        std::unordered_map<char, Node *> childNodes; // TODO: reserve (?)

        Node(Node *parent, char *text, uint length) : parent(parent),
                                                      text(text),
                                                      length(length),
                                                      nodeType(NodeType::LIGHT) {}
    };
}