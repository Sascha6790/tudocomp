#pragma once

#include <cmath>
#include <concepts>
#include <tudocomp/compressors/lz77/ds/model/Node.hpp>

namespace tdc::lz77 {

    template<typename T> requires std::integral<T>
    class WeightedSuffixTree {
        int *lcpArray;
        int *suffixArray;
        const char *buffer;
        int size;
        WeightedNode <T> *root;
        int currentIteration = 0;
        WeightedNode <T> *rightmostLeaf;
    public:
        WeightedSuffixTree(int *lcpArray, int *suffixArray, const char *buffer, const int size) : lcpArray(lcpArray),
                                                                                                  suffixArray(
                                                                                                          suffixArray),
                                                                                                  buffer(buffer),
                                                                                                  size(size) {
            root = new WeightedNode<T>(nullptr);
            this->rightmostLeaf = root;
            root->edgeLabelLength = 0;

            //add first node to be able to check for lcp[i-1] (i >= 1)
            addLeaf(root);
            this->currentIteration++;

            for (; this->currentIteration < size; this->currentIteration++) {
                WeightedNode<T> *deepestNode = this->rightmostLeaf;

                while (!isDeepestNode(deepestNode)) {
                    deepestNode = deepestNode->parent;
                }

                if (requiresSplit(deepestNode)) {
                    deepestNode = splitNode(deepestNode);
                }

                addLeaf(deepestNode);

                return;
            }
        }

        WeightedNode <T> *getRoot() const {
            return root;
        }

        WeightedNode <T> *splitNode(WeightedNode <T> *deepestNode) {
            WeightedNode<T> *v = deepestNode;
            WeightedNode<T> *w = deepestNode->rightmost;
            if (!w) {
                return v;
            }

            // 1. Delete (v,w)
            v->childNodes.erase(w->edgeLabel[0]);
            w->parent = nullptr;

            // 2. Add a new node y and a new edge (v, y)
            auto *y = new WeightedNode<T>(v);
            uint startOfString = suffixArray[currentIteration - 1] + v->depth;
            uint endOfString = suffixArray[currentIteration - 1] + lcpArray[currentIteration] - 1;
            y->edgeLabel = &buffer[startOfString];
            y->edgeLabelLength = endOfString - startOfString + 1;
            y->depth = v->depth + y->edgeLabelLength;
            v->childNodes[y->edgeLabel[0]] = y;

            // 3. Add (y,w)
            startOfString = suffixArray[currentIteration - 1] + lcpArray[currentIteration];

            endOfString = suffixArray[currentIteration - 1] + w->depth - 1;

            this->rightmostLeaf = w;
            y->rightmost = w;
            w->parent = y;

            // leaf->depth = splitNode->depth + splitNode->edgeLabelLength;
            w->edgeLabel = &buffer[startOfString];
            w->edgeLabelLength = endOfString - startOfString + 1;


            y->childNodes[buffer[startOfString]] = w;
            setDepth(w);
            updateMinMaxBottomUp(w, w->nodeLabel);

            return y;
        }

        virtual ~WeightedSuffixTree() {
            destructPointers(root); // (valgrind) using pointers in maps require manually destruction of each pointer.
            delete root;
        }

        void destructPointers(WeightedNode <T> *parent) {
            auto itr = parent->childNodes.begin();
            while (itr != parent->childNodes.end()) {
                destructPointers(itr->second);
                delete(itr->second);
                itr = parent->childNodes.erase(itr);
            }
        }

        void addLeaf(WeightedNode <T> *parent) {
            int startOfString = suffixArray[currentIteration] + lcpArray[currentIteration];
            int endOfString = size;

            // latest child equals right-most
            parent->childNodes[buffer[startOfString]] = new WeightedNode(parent);
            auto leaf = parent->childNodes[buffer[startOfString]];
            parent->rightmost = leaf;
            this->rightmostLeaf = leaf;
            leaf->parent = parent;

            leaf->edgeLabel = &buffer[startOfString];
            leaf->edgeLabelLength = endOfString - startOfString;
            leaf->nodeLabel = suffixArray[currentIteration];

            setDepth(leaf);
            updateMinMaxBottomUp(leaf, leaf->nodeLabel);
        }

        void updateMinMaxBottomUp(WeightedNode <T> *node, T label) {
            if (label > node->maxLabel) {
                node->maxLabel = label;
            }
            if (label < node->minLabel) {
                node->minLabel = label;
            }
            if (node->parent != nullptr) {
                updateMinMaxBottomUp(node->parent, label); // O(\sigma)
            }
        }

        bool isDeepestNode(WeightedNode <T> *node) {
            return node->depth <= lcpArray[currentIteration];
        }

        void setDepth(WeightedNode <T> *leaf) {
            leaf->depth = leaf->parent->depth + leaf->edgeLabelLength;
        }

        bool requiresSplit(WeightedNode <T> *node) {
            return node->depth < lcpArray[currentIteration];
        }
    };
}