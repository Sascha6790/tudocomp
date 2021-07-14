#pragma once

#include <cmath>
#include <concepts>
#include <tudocomp/compressors/lz77/ds/model/Node.hpp>
#include <stdexcept>

// TODO Move Custom LCP Construction to the class and remove from constructor.
namespace tdc::lz77 {

    template<typename T> requires std::integral<T>
    class CappedWeightedSuffixTree {
        int *lcpArray;
        int *suffixArray;
        const char *buffer;
        int size;
        const int fixedLength;
        WeightedNode<T> *root;
        int currentIteration = 0;
    public:
        CappedWeightedSuffixTree(int *lcpArray,
                                 int *suffixArray,
                                 const char *buffer,
                                 const int size,
                                 const int fixedLength) : lcpArray(lcpArray),
                                                          suffixArray(suffixArray),
                                                          buffer(buffer),
                                                          size(size),
                                                          fixedLength(fixedLength) {
            guard();
            initRootNode();
            for (; currentIteration < size; currentIteration++) {
                if (isSuffixWithinBounds()) {
                    postProcessLcpArray(); // Post-Process LCP Array
                    continue;
                }
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

        void initRootNode() {
            root = new WeightedNode<T>(nullptr);
            root->rightmost = root;
        }

        void guard() const {
            if (fixedLength * 2 > size) {
                throw std::invalid_argument("fixedLength * 2 > size");
            }
        }

        /*
         * The LCP-Array[i+1] stores the longest common prefixes of SA[i] and SA[i+1].
         * If SA[i] + fixedSize < dsSize, we don't want to add SA[i] to the suffix tree
         * because it's length doesn't suffice the given criteria and thus
         * SA[i+1] has no common prefix with SA[i]
         */
        void postProcessLcpArray() {
            lcpArray[currentIteration + 1] = 0;
        }

        bool isSuffixWithinBounds() const {
            return suffixArray[currentIteration] + fixedLength > size;
        }

        WeightedNode<T> *getRoot() const {
            return root;
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
            w->edgeLabel = &buffer[suffixArray[currentIteration - 1] + getLcpValue()];
            w->edgeLabelLength = (suffixArray[currentIteration - 1] + y->depth) -
                                 (suffixArray[currentIteration - 1] + getLcpValue() - 1);
            w->depth = y->depth + w->edgeLabelLength;
            y->childNodes[w->edgeLabel[0]] = w;
            updateMinMaxBottomUp(w->parent, w->nodeLabel);
            return w;
        }

        WeightedNode<T> *getSplitMiddleNode(WeightedNode<T> *v) {
            auto *y = new WeightedNode<T>(v);
            y->edgeLabel = &buffer[suffixArray[currentIteration - 1] + v->depth];
            y->edgeLabelLength = (suffixArray[currentIteration - 1] + getLcpValue()) -
                                 (suffixArray[currentIteration - 1] + v->depth);
            y->depth = v->depth + y->edgeLabelLength;
            v->childNodes[y->edgeLabel[0]] = y;
            return y;
        }

        virtual ~CappedWeightedSuffixTree() {
            delete root;
        }

        void addLeaf(WeightedNode<T> *parent) {
            if (fixedLength - parent->depth < 1) {
                return;
            }
            parent->childNodes[buffer[suffixArray[currentIteration] + getLcpValue()]] = new WeightedNode(
                    parent);
            // latest child equals right-most
            root->rightmost = parent->childNodes[buffer[suffixArray[currentIteration] + getLcpValue()]];
            parent->rightmost = root->rightmost;
            setLabelsForLeaf(root->rightmost);
            updateMinMaxBottomUp(root->rightmost, root->rightmost->nodeLabel);
        }

        void setLabelsForLeaf(WeightedNode<T> *leaf) {
            int maxLabelLengthAllowed = fixedLength - leaf->parent->depth;
            leaf->nodeLabel = suffixArray[currentIteration];
            leaf->edgeLabel = &buffer[suffixArray[currentIteration] + getLcpValue()];
            leaf->edgeLabelLength = std::min(maxLabelLengthAllowed,
                                             size - (suffixArray[currentIteration] + getLcpValue()));
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
            return node->depth <= getLcpValue();
        }

        int getLcpValue() {
            return std::min(lcpArray[currentIteration], fixedLength);
        }

        bool requiresSplit(WeightedNode<T> *node) {
            return node->depth < getLcpValue();
        }
    };
}