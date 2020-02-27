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
				
				if (temp->treap.getSize() >= 64)
				{
					Node *left = new Node(val);
					left->val = NULL;
					left->isRoute = false;
					
					Node *right = new Node(val);
					right->val = NULL;
					right->isRoute = false;
					
					int headval = temp->treap.nodes[temp->treap.root].val;
					
					temp->treap.split(left->treap, right->treap);
					
					temp->val = headval;
					temp->isRoute = true;
					temp->left = left;
					temp->right = right;
					// Leave this node's treap in the struct. 
				}
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
		Node *temp2;

        while (true) {
			if (temp.isRoute == false)
			{
				temp->treap.immutableRemove(val);
				
				if (temp->treap.getSize() <= 16)
				{										
					if (temp2->left == temp)
					{
						if (temp2->right != NULL && temp2->right->treap.getSize() <= 16)
						{
							temp->treap = temp->treap.merge(temp, temp2->right);
						}
					}
						
					else
					{
						if (temp2->left != NULL && temp2->left->treap.getSize() <= 16)
						{
							temp->treap = temp->treap.merge(temp2->left, temp);
						}
							
					}
				}
				return;
			}
			
			
            if (temp->val > val) {
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

