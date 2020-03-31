#include <iostream>
#include <limits>
#include <mrlock.h>

#include "searchtree.h"

const int Empty = numeric_limits<int>::min();
const int TreapSplitThreshold = 64;
const int TreapMergeThreshold = 32;

using namespace std;

SearchTree::SearchTree() : mrlock(1) {
    // Set up the initial head as a base node
    head = new Node(Empty);
    head->treap = Treap::New();
    head->isRoute = false;

    // Set up the tree lock
    treeLock.Resize(1);
    treeLock.Set(1);
}

void SearchTree::insert(int val) {
    // Acquire the lock
    ScopedMrLock lock(&mrlock, treeLock);

    Node *temp = head;

    // Search until a base node is found
    while (temp->isRoute) {
        if (temp->val >= val) {
            temp = temp->left;
        }
        else {
            temp = temp->right;
        }
    }

    // Insert the value
    Treap * expected = temp->treap.load();
    Treap * desired = temp->treap.load()->immutableInsert(val);
    while (!atomic_compare_exchange_weak(&temp->treap, &expected, desired)) {
        desired = temp->treap.load()->immutableInsert(val);
    }

    // If inserting causes the treap to become too large, split it in two
    if (temp->treap.load()->getSize() >= TreapSplitThreshold) {
        Node *left = new Node(Empty);
        left->isRoute = false;

        Node *right = new Node(Empty);
        right->isRoute = false;
        Treap* lefttemp = left->treap.load();
        Treap* righttemp = right->treap.load();
        
        int splitVal = temp->treap.load()->split(&lefttemp, &righttemp);

        temp->val = splitVal;
        temp->isRoute = true;
        temp->left = left;
        temp->right = right;

        delete(temp->treap);
        temp->treap = nullptr;
    }
}

void SearchTree::remove(int val) {
    // Acquire the lock
    ScopedMrLock lock(&mrlock, treeLock);

    Node *temp = head;
    Node *tempParent = nullptr;

    // Search until a base node is found
    while (temp->isRoute) {
        if (temp->val >= val) {
            tempParent = temp;
            temp = temp->left;
        }
        else {
            tempParent = temp;
            temp = temp->right;
        }
    }

    // Perform the remove
    Treap * expected = temp->treap.load();
    Treap * desired = temp->treap.load()->immutableInsert(val);
    while (!atomic_compare_exchange_weak(&temp->treap, &expected, desired)) {
        desired = temp->treap.load()->immutableInsert(val);
    }

    // Check if a merge is possible. This is when the node has a parent, and the node's sibling is also a base node
    bool mergeIsPossible = tempParent != nullptr && !tempParent->left->isRoute && !tempParent->right->isRoute;

    if (mergeIsPossible) {
        // Check if the two nodes are small enough to be merged
        int combinedSize = tempParent->left->treap.load()->getSize() + tempParent->right->treap.load()->getSize();
        if (combinedSize <= TreapMergeThreshold) {
            tempParent->treap = Treap::merge(tempParent->left->treap, tempParent->right->treap);
            tempParent->isRoute = false;

            delete(tempParent->left->treap);
            delete(tempParent->left);
            tempParent->left = nullptr;

            delete(tempParent->right->treap);
            delete(tempParent->right);
            tempParent->right = nullptr;
        }
    }
}

bool SearchTree::lookup(int val) {
    // Acquire the lock
    ScopedMrLock lock(&mrlock, treeLock);

    Node *temp = head;

    // Search until a base node is found
    while (temp->isRoute) {
        if (temp->val >= val) {
            temp = temp->left;
        }
        else {
            temp = temp->right;
        }
    }

    return temp->treap.load()->contains(val);
}

vector<int> SearchTree::rangeQuery(int low, int high) {
    // Acquire the lock
    ScopedMrLock lock(&mrlock, treeLock);

    vector<int> result;
    vector<Node *> nodesToCheck;
    nodesToCheck.push_back(head);

    while (!nodesToCheck.empty()) {
        Node *temp = nodesToCheck.back();
        nodesToCheck.pop_back();

        // If popped node is base node, perform range query on treap and add it to result.
        if (!temp->isRoute) {
            vector<int> values = temp->treap.load()->rangeQuery(low, high);
            result.insert(result.end(), values.begin(), values.end());
            continue;
        }

        // Check values to the left if this value is not smaller than the minimum value
        if (temp->val >= low) {
            nodesToCheck.push_back(temp->left);
        }

        // Check values to the right if this value is not larger than the maximum value
        if (temp->val <= high) {
            nodesToCheck.push_back(temp->right);
        }
    }

    return result;
}
