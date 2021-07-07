#pragma once

#include <cmath>
#include <tudocomp/compressors/lz77/ds/model/Node.hpp>

/**
 * Nodes are classified in NodeType::HEAVY and NodeType::LIGHT
 * Nodes are of type HEAVY if they have at least \Theta(lg^2 lg \sigma) =: heavyThreshold leaves.
 */
namespace tdc {
    namespace lz77 {

        class WExponentialSearchTree {
            unsigned int heavyThreshold;
            unsigned int alphabetSize;
            Node *root;

        public:
            explicit WExponentialSearchTree(unsigned int alphabetSize) {
                this->alphabetSize = alphabetSize;
                this->heavyThreshold = floor(pow(log2(log2(static_cast<double>(this->alphabetSize))), 2.0));
                this->root = new Node(nullptr, nullptr, 0);
                std::cout << "Heavy nodes have at least " << this->heavyThreshold << " leaves." << std::endl;
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
                // Exit strategy
                if (elementLength == 0) {
                    return;
                }

                uint matchingChars = getLcp(rootNode, element, elementLength);

                auto unmatchedElementChars = &element[matchingChars];
                if (rootNode->childNodes.find(*unmatchedElementChars) == rootNode->childNodes.end()) {
                    if (matchingChars == 0) {
                        if(rootNode->length > 1) {
                            // Given pi$:  ppi$ zu  p -> [pi$, i$]
                            rootNode->childNodes[rootNode->text[1]] = new Node(rootNode, &rootNode->text[1], rootNode->length - 1);
                        }
                        rootNode->length = 1;
                        rootNode->childNodes[*unmatchedElementChars] = new Node(rootNode, unmatchedElementChars,
                                                                                    elementLength);
                    } else {
                        // matchingChars != root->length -1 -> split required
                        if (rootNode->childNodes.empty()) {
                            // current node is a Leaf
                            // Given String: NAX, Given Node: NANAX zu NA -> [NAX, X]
                            rootNode->childNodes.emplace( // emplace without hint: O(log(N)) (faster than insert or [])
                                    std::make_pair(static_cast<char>(rootNode->text[matchingChars + 1]),
                                                   new Node(rootNode, &rootNode->text[matchingChars + 1],
                                                            rootNode->length - matchingChars - 1))); // NAX
                            rootNode->childNodes.emplace(
                                    std::make_pair(static_cast<char>(*unmatchedElementChars),
                                                   new Node(rootNode, unmatchedElementChars,
                                                            elementLength - matchingChars))); // X
                            rootNode->length = matchingChars + 1; //shorten current node
                        } else {
                            std::cerr << "ERROR, NOT A MATCHED CASE (2)" << std::endl;
                            exit(-1);
                        }
                    }
                } else { // entry found in unsorted_map
                    if (rootNode->length == 1) {
                        addToNode(rootNode->childNodes[*unmatchedElementChars], &element[1],
                                  elementLength - rootNode->length);
                    } else {
                        // Add AX to ANA -> [X, NAX].
                        // Can't add X to [X, NAX] because we would omit "NA" from "ANA" (length == 3 != 1)
                        // Split ANA to A -> NA -> [X, NAX] and add A -> X to the root resulting in A -> [X, NA -> [X, NAX]]
                        // rootNode->length = matchingChars + 1;

                        rootNode->parent->childNodes.erase(
                                rootNode->text[0]); // remove reference root -> A //TODO does insert remove/replace the object ? might omit erase in that case.
                        auto replacementNode = rootNode->parent->childNodes.insert( // A -> NA
                                std::make_pair( //create a new object A and reference it in root.
                                        static_cast<char>(rootNode->text[0]),
                                        new Node(rootNode->parent, rootNode->text, matchingChars + 1)
                                ));

                        rootNode->parent = replacementNode.first->second;
                        rootNode->length = rootNode->length - matchingChars - 1;
                        rootNode->text = &rootNode->text[matchingChars + 1];
                        replacementNode.first->second->childNodes.insert(
                                std::make_pair(static_cast<char>(rootNode->text[0]), rootNode)); // NA -> [X, NAX]
                        addToNode(rootNode->parent, unmatchedElementChars,
                                  elementLength - matchingChars);// TODO: A -> X
                    }
                }
            }

            //naive lcp
            static uint getLcp(const Node *root, const char *element, size_t elementSize) {
                uint matchingChars = 0;
                if (root->text != nullptr) {
                    for (int i = 1; i < std::min(elementSize, root->length); i++) {
                        if (element[i - 1] == root->text[i]) {
                            ++matchingChars;
                        } else {
                            break;
                        }
                    }
                }
                return matchingChars;
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