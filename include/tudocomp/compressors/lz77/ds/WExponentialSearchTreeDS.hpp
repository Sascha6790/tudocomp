#pragma once

#include <math.h>

namespace tdc {
    namespace lz77 {
        class WExponentialSearchTreeDS {
        public:
            WExponentialSearchTreeDS(unsigned int alphabetSize = 255, unsigned int dataSize) {
                this->alphabetSize = alphabetSize;
                this->treeSize = dataSize;
                this->heavyThreshold = floor(pow(log2(log2(static_cast<double>(this->alphabetSize))), 2.0));
                this->root = new Node(0);
                std::cout << "Heavy nodes have at least " << s << " leaves." << std::endl;
            }

        private:
            unsigned int heavyThreshold;
            unsigned int alphabetSize;
            unsigned int treeSize;
            Node root;

            void addEdge(Node * parent, Node * child) {
                parent->childNodes[child->firstChar] = child;
                if(parent->childNodes.size() > this->s) {
                    parent->nodeType = NodeType::HEAVY;
                }
            }
            enum NodeType {
                HEAVY,
                LIGHT
            };

            struct Node {
                Node * parent;
                char firstChar;
                NodeType nodeType;
                std::unordered_map<char, Node*> childNodes; // TODO: reserve (?)

                Node(Node * parent = nullptr, char firstChar) : parent(parent), nodeType(NodeType::LIGHT), firstChar(firstChar) {}
            };

            struct Leaf : Node {
                uint suffix;

                Leaf(int s, int e = 0) : Node(s, e) {}
            };

        };
    }
}
