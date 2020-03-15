#include <iostream>
#include <limits>

#include "treap.h"
#include "searchtree.h"

const int Empty = numeric_limits<int>::min();
const int TreapSplitThreshold = 64;
const int TreapMergeThreshold = 32;

using namespace std;

void SearchTree::insert(int val) {
    if (head == NULL) {
        Node *n = new Node(Empty);

        n->treap = new Treap();
        n->treap = n->treap->immutableInsert(val);
        n->isRoute = false;

        head = n;
        return;
    }

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
    temp->treap = temp->treap->immutableInsert(val);

    // If inserting causes the treap to become too large, split it in two
    if (temp->treap->getSize() >= TreapSplitThreshold) {
        Node *left = new Node(Empty);
        left->isRoute = false;

        Node *right = new Node(Empty);
        right->isRoute = false;

        int headVal = temp->treap->getRoot();

        temp->treap->split(&left->treap, &right->treap);

        temp->val = headVal;
        temp->isRoute = true;
        temp->left = left;
        temp->right = right;

        delete(temp->treap);
        temp->treap = NULL;
    }
}

void SearchTree::remove(int val) {
    if (head == NULL) {
        return;
    }

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
    temp->treap = temp->treap->immutableRemove(val);

    // Check if a merge is possible. This is when the node has a parent, and the node's sibling is also a base node
    bool mergeIsPossible = tempParent != nullptr && !tempParent->left->isRoute && !tempParent->right->isRoute;

    if (mergeIsPossible) {
        // Check if the two nodes are small enough to be merged
        int combinedSize = tempParent->left->treap->getSize() + tempParent->right->treap->getSize();
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
    if (head == NULL) {
        return false;
    }

    Node *temp = head;

    while (true) {
        // Travels down until it finds the correct base node, performs contains on the treap once it does.
        if (temp->isRoute == false)
        {
            return temp->treap->contains(val);
        }

        if (temp->val >= val) {
            if (temp->left == NULL) {
                return false;
            }
            else {
                temp = temp->left;
            }
        }
        else {
            if (temp->right == NULL) {
                return false;
            }
            else {
                temp = temp->right;
            }
        }
    }
}

vector<int> SearchTree::rangeQuery(int low, int high) {
    vector<int> result;
    if (head == NULL) {
        return result;
    }
    Node* temp = head;
    vector<Node*> nodesToCheck;
    nodesToCheck.push_back(temp);
    vector<int> values;

    while (!nodesToCheck.empty()) {
        temp = nodesToCheck.back();
        nodesToCheck.pop_back();

        // If popped node is base node, perform range query on treap and add it to result.
        if (temp->isRoute == false) {
            values = temp->treap->rangeQuery(low, high);
            result.insert(result.end(), values.begin(), values.end());
            continue;
        }

        Node* currentLeft = temp->left;
        Node* currentRight = temp->right;

        // Adds left child if within range
        if (currentLeft != NULL && temp->val >= low) {
            nodesToCheck.push_back(currentLeft);
        }
        // Adds right child if within range
        if (currentRight != NULL && temp->val <= high) {
            nodesToCheck.push_back(currentRight);
        }
        // Catches minimum boundary case
        if (currentLeft != NULL && currentRight != NULL && currentLeft->val <= low && currentRight->val >= low) {
            nodesToCheck.push_back(currentLeft->right);
        }
        // Catches maximum boundary case
        if (currentLeft != NULL && currentRight != NULL && currentLeft->val <= high && currentRight->val >= high) {
            nodesToCheck.push_back(currentRight->left);
        }
    }
    
    return result;
}
