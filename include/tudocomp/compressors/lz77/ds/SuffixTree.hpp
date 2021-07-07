#pragma once

#include <cmath>
#include <tudocomp/compressors/lz77/ds/model/Node.hpp>

/**
 * Nodes are classified in NodeType::HEAVY and NodeType::LIGHT
 * Nodes are of type HEAVY if they have at least \Theta(lg^2 lg \sigma) =: heavyThreshold leaves.
 */
namespace tdc {
    namespace lz77 {

        class SuffixTree {
            unsigned int heavyThreshold;
            unsigned int alphabetSize;
            Node *root;

        public:
            explicit SuffixTree() {

            }

            /**
             * Adds the given string to the root-node element of the tree.
             *
             * @param element the string to process
             * @param elementSize length of the given string
             */
            void add(char *element, size_t elementSize) {
                addToNode(this->root, element, elementSize);
            }

            /**
             * Adds the given string to the specified node of the tree.
             *
             * @param rootNode Node that's used as root for traversing
             * @param element the string to process
             * @param elementLength length of the given string
             */
            void addToNode(Node *rootNode, char *element, size_t elementLength) {

            }

            Node *getTree() {
                return this->root;
            }

            void static printChildren(const std::unordered_map<char, Node *> &elements) {
                std::cout << "Node( ";
                for (auto node : elements) {
                    std::cout << "[";
                    for (int i = 0; i < node.second->length; i++) {
                        std::cout << node.second->text[i];
                    }

                    if (!node.second->childNodes.empty()) {
                        std::cout << "--> ";
                        printChildren(node.second->childNodes);
                    }
                    std::cout << "]";
                }
                std::cout << " ) ";
            }
        };
    }
}