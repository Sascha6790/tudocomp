#pragma once

#include <cmath>
#include <concepts>
#include <tudocomp/compressors/lz77/ds/model/Node.hpp>
#include <stdexcept>

// TODO Move Custom LCP Construction to the class and remove from constructor.
namespace tdc::lz77 {

    template<typename T> requires std::integral<T>
    class CappedWeightedSuffixTree {
        int *lcpArray{};
        int *suffixArray{};
    private:

        uint size;
        const int window;
        WeightedNode<T> *root;
        int currentIteration = 0;
        WeightedNode<T> *rightmostLeaf;
    public:
        const char *buffer;
        CappedWeightedSuffixTree(int *lcpArray,
                                 int *suffixArray,
                                 const char *buffer,
                                 const uint size,
                                 const uint window) : lcpArray(lcpArray),
                                                      suffixArray(suffixArray),
                                                      size(size),
                                                      buffer(buffer),
                                                      window(window) {

            guard();
            initRootNode();
            for (; currentIteration < size; currentIteration++) {
                if (!isSuffixWithinBounds()) {
                    postProcessLcpArray(); // Post-Process LCP Array
                    continue;
                }
                WeightedNode<T> *deepestNode = this->rightmostLeaf;

                while (isNotDeepestNode(deepestNode)) {
                    deepestNode = deepestNode->parent;
                }

                if (requiresSplit(deepestNode)) {
                    deepestNode = splitNode(deepestNode);
                }

                addLeaf(deepestNode);
            }
        }

        [[gnu::always_inline]]
        inline void initRootNode() {
            root = new WeightedNode<T>(nullptr);
            rightmostLeaf = root;
            root->edgeLabelLength = 0;
            root->rightmost = root;
        }

        [[gnu::always_inline]]
        inline void guard() const {
            if (window * 2 - 1 > size) {
                throw std::invalid_argument("fixedLength * 2 - 1 > size");
            }
        }

        /*
         * The LCP-Array[i+1] stores the longest common prefixes of SA[i] and SA[i+1].
         * If SA[i] + fixedSize < dsSize, we don't want to add SA[i] to the suffix tree
         * because it's length doesn't suffice the given criteria and thus
         * SA[i+1] has no common prefix with SA[i]
         */
        [[gnu::always_inline]]
        inline void postProcessLcpArray() {
            if(currentIteration + 1 < size) {
                lcpArray[currentIteration + 1] = std::min(lcpArray[currentIteration], lcpArray[currentIteration + 1]);
            }
            suffixArray[currentIteration] = currentIteration == 0 ? 0 : suffixArray[currentIteration - 1];
        }

        inline bool isSuffixWithinBounds() const {
            return suffixArray[currentIteration] < window;
        }

        WeightedNode<T> *getRoot() const {
            return root;
        }

        [[gnu::always_inline]][[gnu::hot]]
        inline WeightedNode<T> *splitNode(WeightedNode<T> *deepestNode) {
            WeightedNode<T> *v = deepestNode;
            WeightedNode<T> *w = deepestNode->rightmost;
            if (!w) {
                // updateMinMaxBottomUp(deepestNode, suffixArray[currentIteration]);
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
            v->rightmost = y;
            y->rightmost = w;
            y->nodeLabel = w->nodeLabel;
            w->parent = y;

            // leaf->depth = splitNode->depth + splitNode->edgeLabelLength;
            w->edgeLabel = &buffer[startOfString];
            w->edgeLabelLength = endOfString - startOfString + 1;


            y->childNodes[buffer[startOfString]] = w;
            setDepth(w);
            updateMinMaxBottomUp(w, w->nodeLabel);

            return y;
        }

        virtual ~CappedWeightedSuffixTree() {
            destructPointers(root); // (valgrind) using pointers in maps require manually destruction of each pointer.
            delete root;
        }

        void destructPointers(WeightedNode<T> *parent) {
            if(parent == nullptr) {
                return;
            }
            auto itr = parent->childNodes.begin();
            while (itr != parent->childNodes.end()) {
                if(itr->second != nullptr) {
                    destructPointers(itr->second);
                    delete (itr->second);
                }
                itr = parent->childNodes.erase(itr);
            }
        }

        [[gnu::always_inline]][[gnu::hot]]
        inline void addLeaf(WeightedNode<T> *parent) {
            if (window - parent->depth < 1) {
                updateMinMaxBottomUp(parent, suffixArray[currentIteration]);
                return;
            }
            int startOfString = suffixArray[currentIteration] + lcpArray[currentIteration];
            int endOfString = size;


            // latest child equals right-most
            parent->childNodes[buffer[startOfString]] = new WeightedNode(parent);
            auto leaf = parent->childNodes[buffer[startOfString]];
            uint maxLabelLengthAllowed = window - leaf->parent->depth;
            parent->rightmost = leaf;
            this->rightmostLeaf = leaf;
            leaf->parent = parent;

            leaf->edgeLabel = &buffer[startOfString];
            leaf->edgeLabelLength = endOfString - startOfString;
            leaf->edgeLabelLength = std::min(maxLabelLengthAllowed, leaf->edgeLabelLength); // cap
            leaf->nodeLabel = suffixArray[currentIteration];

            setDepth(leaf);
            updateMinMaxBottomUp(leaf, leaf->nodeLabel);
        }

        // inline a recursion ? Outcome heavily depends on the compiler and probably gets ignored anyway.
        // some (e.g. MSVC) have an optimized setting which limits the maximum depth.
        // remark: tested -> inline improves performance on a small text. not tested on big texts.
        inline void updateMinMaxBottomUp(WeightedNode<T> *node, T label) {
            if (label > node->maxLabel) {
                node->maxLabel = label;
            }
            if (label < node->minLabel) {
                node->minLabel = label;
            }
            if (node->parent != nullptr) {
                updateMinMaxBottomUp(node->parent, label);
            }
        }

        [[gnu::always_inline]]
        inline bool isNotDeepestNode(WeightedNode<T> *node) {
            return node->depth > lcpArray[currentIteration];
        }

        [[gnu::always_inline]]
        inline void setDepth(WeightedNode<T> *leaf) {
            leaf->depth = leaf->parent->depth + leaf->edgeLabelLength;
        }

        [[gnu::always_inline]]
        inline bool requiresSplit(WeightedNode<T> *node) {
            return node->depth < lcpArray[currentIteration];
        }

        void print(WeightedNode<T> *parent) {
            for (int i = 0; i < parent->edgeLabelLength; i++) {
                std::cout << parent->edgeLabel[i];
            }
            if (parent->edgeLabelLength) {
                std::cout << "(" << parent->minLabel << "," << parent->maxLabel << ")";
            }
            auto itr = parent->childNodes.begin();
            bool hasChildren = itr != parent->childNodes.end();
            if (hasChildren) {
                std::cout << "->Children(";
            }
            while (itr != parent->childNodes.end()) {
                std::cout << "[";
                print(itr->second);
                std::cout << "]";
                itr++;
                if (itr != parent->childNodes.end()) {
                    std::cout << ", ";
                }
            }
            if (hasChildren) {
                std::cout << ") ";
            }
        }
    };
}