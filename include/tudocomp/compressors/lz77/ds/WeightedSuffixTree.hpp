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
        WeightedNode<T> *root;
        int currentIteration = 0;
    public:
        WeightedSuffixTree(int *lcpArray, int *suffixArray, const char *buffer, const int size) : lcpArray(lcpArray),
                                                                                                  suffixArray(
                                                                                                          suffixArray),
                                                                                                  buffer(buffer),
                                                                                                  size(size) {
            root = new WeightedNode<T>(nullptr);
            root->rightmost = root;

            for (; this->currentIteration < size; this->currentIteration++) {
                WeightedNode<T> *deepestNode = root->rightmost;

                while (!isDeepestNode(deepestNode)) {
                    deepestNode = deepestNode->parent;
                }

                if (requiresSplit(deepestNode)) {
                    deepestNode = splitNode(deepestNode);
                }

                addLeaf(deepestNode);
            }
        }

        WeightedNode<T> *splitNode(WeightedNode<T> *deepestNode) {
            WeightedNode<T> *v = deepestNode;
            WeightedNode<T> *w = deepestNode->rightmost;

            // 1. Delete (v,w)
            v->childNodes.erase(w->edgeLabel[0]);

            // 2. Add a new node y and a new edge (v, y)
            WeightedNode<T> *y = getSplitMiddleNode(v);

            // 3. Add (y, w)
            updateSplittedNode(w, y);
            return y;
        }

        WeightedNode<T> *updateSplittedNode(WeightedNode<T> *w, WeightedNode<T> *y) {
            w->parent = y;
            w->edgeLabel = &buffer[suffixArray[currentIteration - 1] + lcpArray[currentIteration]];
            w->edgeLabelLength = (suffixArray[currentIteration - 1] + y->depth) -
                                 (suffixArray[currentIteration - 1] + lcpArray[currentIteration] - 1);
            w->depth = y->depth + w->edgeLabelLength;
            y->childNodes[w->edgeLabel[0]] = w;
            updateMinMaxBottomUp(w->parent, w->nodeLabel);
            return w;
        }

        WeightedNode<T> *getSplitMiddleNode(WeightedNode<T> *v) {
            auto *y = new WeightedNode<T>(v);
            y->edgeLabel = &buffer[suffixArray[currentIteration - 1] + v->depth];
            y->edgeLabelLength = (suffixArray[currentIteration - 1] + lcpArray[currentIteration]) -
                                 (suffixArray[currentIteration - 1] + v->depth);
            y->depth = v->depth + y->edgeLabelLength;
            v->childNodes[y->edgeLabel[0]] = y;
            return y;
        }

        virtual ~WeightedSuffixTree() {
            delete root;
        }

        void addLeaf(WeightedNode<T> *parent) {
            parent->childNodes[buffer[suffixArray[currentIteration] + lcpArray[currentIteration]]] = new WeightedNode(
                    parent);
            // latest child equals right-most
            root->rightmost = parent->childNodes[buffer[suffixArray[currentIteration] + lcpArray[currentIteration]]];
            parent->rightmost = root->rightmost;
            setLabelsForLeaf(root->rightmost);
            updateMinMaxBottomUp(root->rightmost, root->rightmost->nodeLabel);
        }

        void setLabelsForLeaf(WeightedNode<T> *leaf) {
            leaf->nodeLabel = suffixArray[currentIteration];
            leaf->edgeLabel = &buffer[suffixArray[currentIteration] + lcpArray[currentIteration]];
            leaf->edgeLabelLength = size - (suffixArray[currentIteration] + lcpArray[currentIteration]);
            leaf->depth = leaf->parent->depth + leaf->edgeLabelLength;
        }

        void updateMinMaxBottomUp(WeightedNode<T> *node, T label) {
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

        bool isDeepestNode(WeightedNode<T> *node) {
            return node->depth <= lcpArray[currentIteration];
        }

        bool requiresSplit(WeightedNode<T> *node) {
            return node->depth < lcpArray[currentIteration];
        }
    };
}