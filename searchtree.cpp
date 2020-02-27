#include <iostream>
#include "treap.h"

using namespace std;

template <class T>
class SearchTree {
private:
    class Node {
    public:
        bool isRoute {true};
		T val;
		int weight; 
		Treap* treap {NULL};
		
        Node *left {NULL};
        Node *right {NULL};

        Node(T val) {
            this->val = val;
			this->weight = weightDist(randEngine);
        }
    };

    Node *head {NULL};
	
public:
    void insert(T val) {
        if (head == NULL) {
            Node *n = new Node(val);
			
			n->treap = new Treap();
			n->treap.immutableInsert(val);
			n->val = NULL;
			n->isRoute = false;
			
            head = n;
            return;
        }

        Node *temp = head;

        while (true) {
			if (temp.isRoute == false)
			{
				temp->treap.immutableInsert(val);
				return;
			}
			
			
            if (temp->val > val) {
                if (temp->left == NULL) {
					Node *n = new Node(val);
					
					n->treap = new Treap();
					n->treap.immutableInsert(val);
					n->val = NULL;
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
					Node *n = new Node(val);
					
					n->treap = new Treap();
					n->treap.immutableInsert(val);
					n->val = NULL;
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

    void remove(T val) {
		if (head == NULL) {
            return;
        }

        Node *temp = head;

        while (true) {
			if (temp.isRoute == false)
			{
				temp->treap.immutableRemove(val);
				return;
			}
			
			
            if (temp->val > val) {
                if (temp->left == NULL) {
                    return;
                }
                else {
                    temp = temp->left;
                }
            }
            else {
                if (temp->right == NULL) {
                    return;
                }
                else {
                    temp = temp->right;
                }
            }
        }
    }

    bool lookup(T val) {
		if (head == NULL) {
            return false;
        }

        Node *temp = head;

        while (true) {
			if (temp.isRoute == false)
			{
				return temp->treap.contains(val);
			}
			
			
            if (temp->val > val) {
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
};

int main () {
	
	return 0;
}

