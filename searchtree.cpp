#include <iostream>
#include <limits>

#include "treap.h"
#include "searchtree.h"

const int Empty = numeric_limits<int>::min();
const int TreapSplitSize = 64;


using namespace std;

void SearchTree::insert(int val) {
    if (head == NULL) {
        Node *n = new Node(val);

        n->treap = new Treap();
        n->treap = n->treap->immutableInsert(val);
        n->val = Empty;
        n->isRoute = false;

        head = n;
        return;
    }

    Node *temp = head;

    while (true) {
        // Travels until it finds the base node where the value should go
        if (temp->isRoute == false)
        {
            temp->treap = temp->treap->immutableInsert(val);
            // If inserting causes the treap to become too large, it splits into two.
            if (temp->treap->getSize() >= TreapSplitSize) {
            {
                Node *left = new Node(val);
                left->val = Empty;
                left->isRoute = false;

                Node *right = new Node(val);
                right->val = Empty;
                right->isRoute = false;

                int headval = temp->treap->getRoot();

                temp->treap->split(&left->treap, &right->treap);
                
                temp->val = headval;
                temp->isRoute = true;
                temp->left = left;
                temp->right = right;
                Treap *deletethistreap = temp->treap;
                temp->treap = NULL;
                delete(deletethistreap);
                
                // Leave this node's treap in the struct.
            }
            return;
        }

        if (temp->val >= val) {
            if (temp->left == NULL) {
                // Inserts a new node at this spot if there is no base node here for it.
                Node *n = new Node(val);

                n->treap = new Treap();
                n->treap = n->treap->immutableInsert(val);
                n->val = Empty;
                n->isRoute = false;

                temp->left = n;
                return;
            }
            else {
                temp = temp->left;
            }
        }
        else {
            if (temp->right == NULL) {
                // Inserts a new node at this spot if there is no base node here for it.
                Node *n = new Node(val);

                n->treap = new Treap();
                n->treap = n->treap->immutableInsert(val);
                n->val = Empty;
                n->isRoute = false;

                temp->right = n;
                return;
            }
            else {
                temp = temp->right;
            }
        }
    }
}

void SearchTree::remove(int val) {
    if (head == NULL) {
        return;
    }

    Node *temp = head;
    Node *temp2;

    while (true) {
        // Travels until it finds the base node with the value and performs remove on the treap inside.
        if (temp->isRoute == false)
        {
            temp->treap = temp->treap->immutableRemove(val);
            
            // If that would cause the treap to become too small it performs a merge with temp's sibling.
            if (temp->treap->getSize() <= 16)
            {
                if (temp2->left == temp)
                {
                    if (temp2->right != NULL && temp2->right->treap != NULL && temp2->right->treap->getSize() <= 16)
                    {
                        temp->treap = temp->treap->merge(temp->treap, temp2->right->treap);
                        Node *deletethis = temp2->right;
                        temp2->right = NULL;
                        delete(deletethis->treap);
                        delete(deletethis);
                    }
                }

                else if (temp2->right == temp)
                {
                    if (temp2->left != NULL && temp2->left->treap != NULL && temp2->left->treap->getSize() <= 16)
                    {
                        temp->treap = temp->treap->merge(temp2->left->treap, temp->treap);
                        Node *deletethis = temp2->left;
                        temp2->left = NULL;
                        delete(deletethis->treap);
                        delete(deletethis);
                    }

                }
            }
            return;
        }

        if (temp->val >= val) {
            if (temp->left == NULL) {
                return;
            }
            else {
                temp2 = temp;
                temp = temp->left;
            }
        }
        else {
            if (temp->right == NULL) {
                return;
            }
            else {
                temp2 = temp;
                temp = temp->right;
            }
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
