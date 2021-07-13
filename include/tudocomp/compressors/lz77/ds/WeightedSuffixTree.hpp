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
        Node<T> *root;
        int currentIteration = 0;
    public:
        WeightedSuffixTree(int *lcpArray, int *suffixArray, const char *buffer, const int size) : lcpArray(lcpArray),
                                                                                                  suffixArray(
                                                                                                          suffixArray),
                                                                                                  buffer(buffer),
                                                                                                  size(size) {
            root = new Node<T>(nullptr);
            root->rightmost = root;

            for (; this->currentIteration < size; this->currentIteration++) {
                Node<T> *deepestNode = root->rightmost;

                while (!isDeepestNode(deepestNode)) {
                    deepestNode = deepestNode->parent;
                }

                if (requiresSplit(deepestNode)) {
                    deepestNode = splitNode(deepestNode);
                }

                addLeaf(deepestNode);
            }
        }

        Node<T> *splitNode(Node<T> *deepestNode) const {
            Node<T> *v = deepestNode;
            Node<T> *w = deepestNode->rightmost;

            // 1. Delete (v,w)
            v->childNodes.erase(w->edgeLabel[0]);

            // 2. Add a new node y and a new edge (v, y)
            Node<T> *y = getSplitMiddleNode(v);

            // 3. Add (y, w)
            updateSplittedNode(w, y);
            return y;
        }

        Node<T> *updateSplittedNode(Node<T> *w, Node<T> *y) const {
            w->edgeLabel = &buffer[suffixArray[currentIteration - 1] + lcpArray[currentIteration]];
            w->edgeLabelLength = (suffixArray[currentIteration - 1] + y->depth) -
                                 (suffixArray[currentIteration - 1] + lcpArray[currentIteration] - 1);
            w->depth = y->depth + w->edgeLabelLength;
            y->childNodes[w->edgeLabel[0]] = w;
            return w;
        }

        Node<T> *getSplitMiddleNode(Node<T> *v) const {
            auto *y = new Node<T>(v);
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

        void addLeaf(Node<T> *parent) {
            parent->childNodes[buffer[suffixArray[currentIteration] + lcpArray[currentIteration]]] = new Node(
                    parent);
            // latest child equals right-most
            root->rightmost = parent->childNodes[buffer[suffixArray[currentIteration] + lcpArray[currentIteration]]];
            parent->rightmost = root->rightmost;
            setLabelsForLeaf(root->rightmost);
        }

        void setLabelsForLeaf(Node<T> *leaf) {
            leaf->nodeLabel = suffixArray[currentIteration];
            leaf->edgeLabel = &buffer[suffixArray[currentIteration] + lcpArray[currentIteration]];
            leaf->edgeLabelLength = size - (suffixArray[currentIteration] + lcpArray[currentIteration]);
            leaf->depth = leaf->parent->depth + leaf->edgeLabelLength;
        }

        bool isDeepestNode(Node<T> *node) {
            return node->depth <= lcpArray[currentIteration];
        }

        bool requiresSplit(Node<T> *node) {
            return node->depth < lcpArray[currentIteration];
        }
    };
}