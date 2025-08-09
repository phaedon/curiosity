
#include <cstddef>
#include <iostream>
#include <memory>
#include <unordered_set>

struct BSTNode {
  BSTNode(float v) : val(v) {}

  float val;
  std::unique_ptr<BSTNode> left;
  std::unique_ptr<BSTNode> right;

  void insert(float v) {
    if (v < val) {
      if (left == nullptr) {
        left = std::make_unique<BSTNode>(v);
      } else {
        left->insert(v);
      }
    } else {
      if (right == nullptr) {
        right = std::make_unique<BSTNode>(v);
      } else {
        right->insert(v);
      }
    }
  }

  bool isInRange(float a, float b) const { return a <= val && val <= b; }

  bool isLeaf() const { return left == nullptr && right == nullptr; }

  const BSTNode* find(float v) const {
    if (val == v) return this;
    auto* child = v < val ? left.get() : right.get();
    return child == nullptr ? nullptr : child->find(v);
  }

  void reportSubtree(std::unordered_set<float>& hits) const {
    hits.insert(val);
    if (left != nullptr) left->reportSubtree(hits);
    if (right != nullptr) right->reportSubtree(hits);
  }
};

struct BST {
  std::unique_ptr<BSTNode> root;

  void insert(float v) {
    if (root == nullptr) {
      root = std::make_unique<BSTNode>(v);
    } else {
      root->insert(v);
    }
  }

  const BSTNode* find(float v) const {
    return root == nullptr ? nullptr : root->find(v);
  }

  const BSTNode* findSplitNode(float xmin, float xmax) const {
    if (root == nullptr) return nullptr;

    auto* split = root.get();
    while (split != nullptr && !split->isLeaf() &&
           !split->isInRange(xmin, xmax)) {
      if (xmax < split->val) {
        // The current node is larger than the range.
        // Therefore, the entire range must lie to the left.
        split = split->left.get();
      } else {
        // The entire range lies to the right.
        split = split->right.get();
      }
    }
    return split;
  }

  std::unordered_set<float> rangeQuery1D(float xmin, float xmax) const {
    std::unordered_set<float> hits;

    auto* split = findSplitNode(xmin, xmax);
    if (split == nullptr) {
      return hits;
    }

    if (split->isLeaf()) {
      if (split->val >= xmin && split->val <= xmax) {
        hits.insert(split->val);
      }
    } else {
      if (split->val >= xmin && split->val <= xmax) {
        hits.insert(split->val);
      }

      // follow the path to xmin and report the points in subtrees right of the
      // path
      if (split->left != nullptr) {
        auto* left_subtree = split->left.get();
        while (left_subtree != nullptr && !left_subtree->isLeaf()) {
          if (xmin <= left_subtree->val) {
            hits.insert(left_subtree->val);
            if (left_subtree->right != nullptr) {
              left_subtree->right->reportSubtree(hits);
            }
            left_subtree = left_subtree->left.get();
          } else {
            left_subtree = left_subtree->right.get();
          }
        }
      }

      // follow the path to xmax and report the points in subtrees left of the
      // path
      if (split->right != nullptr) {
        auto* right_subtree = split->right.get();
        while (right_subtree != nullptr && !right_subtree->isLeaf()) {
          if (right_subtree->val <= xmax) {
            hits.insert(right_subtree->val);
            if (right_subtree->left != nullptr) {
              right_subtree->left->reportSubtree(hits);
            }
            right_subtree = right_subtree->right.get();
          } else {
            right_subtree = right_subtree->left.get();
          }
        }
      }
    }

    return hits;
  }
};
