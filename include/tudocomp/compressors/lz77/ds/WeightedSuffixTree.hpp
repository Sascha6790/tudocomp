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

        WeightedNode<T> *splitNode(WeightedNode<T> *deepestNode) const {
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

        WeightedNode<T> *updateSplittedNode(WeightedNode<T> *w, WeightedNode<T> *y) const {
            w->edgeLabel = &buffer[suffixArray[currentIteration - 1] + lcpArray[currentIteration]];
            w->edgeLabelLength = (suffixArray[currentIteration - 1] + y->depth) -
                                 (suffixArray[currentIteration - 1] + lcpArray[currentIteration] - 1);
            w->depth = y->depth + w->edgeLabelLength;
            y->childNodes[w->edgeLabel[0]] = w;
            return w;
        }

        WeightedNode<T> *getSplitMiddleNode(WeightedNode<T> *v) const {
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
            parent->childNodes[buffer[suffixArray[currentIteration] + lcpArray[currentIteration]]] = new Node(
                    parent);
            // latest child equals right-most
            root->rightmost = parent->childNodes[buffer[suffixArray[currentIteration] + lcpArray[currentIteration]]];
            parent->rightmost = root->rightmost;
            setLabelsForLeaf(root->rightmost);
        }

        void setLabelsForLeaf(WeightedNode<T> *leaf) {
            leaf->nodeLabel = suffixArray[currentIteration];
            leaf->edgeLabel = &buffer[suffixArray[currentIteration] + lcpArray[currentIteration]];
            leaf->edgeLabelLength = size - (suffixArray[currentIteration] + lcpArray[currentIteration]);
            leaf->depth = leaf->parent->depth + leaf->edgeLabelLength;
        }

        bool isDeepestNode(WeightedNode<T> *node) {
            return node->depth <= lcpArray[currentIteration];
        }

        bool requiresSplit(WeightedNode<T> *node) {
            return node->depth < lcpArray[currentIteration];
        }
    };
}