#include <iostream>

using namespace std;

template <class T>
class SearchTree {
private:
    class Node {
    public:
        T val;
        Node *left {NULL};
        Node *right {NULL};

        Node(T val) {
            this->val = val;
        }
    };

    Node *head {NULL};
public:
    void insert(T val) {
        Node *n = new Node<T>(val);

        if (head == NULL) {
            head = n;
            return;
        }

        Node *temp = head;

        while (true) {
            if (temp->val > val) {
                if (temp->left == NULL) {
                    temp->left = n;
                    return;
                }
                else {
                    temp = temp->left;
                }
            }
            else {
                if (temp->right == NULL) {
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
        Node *temp = head;
        
        while (true) {
            if (temp->val > val) {
                temp = temp->left;
            }           
            else if (temp->val < val) {
                temp = temp->right;
            }
            else if (temp->val == val) {
                
            }
            else if (temp == NULL) {
                return NULL;
            }
        }
    }

    bool lookup(T val) {
        Node *temp = head;
        
        while (true) {
            if (temp->val > val) {
                temp = temp->left;
            }           
            else if (temp->val < val) {
                temp = temp->right;
            }
            else if (temp->val == val) {
                return true
            }
            else if (temp == NULL) {
                return false;
            }
        }
    
        
    }
};

int main(void) {
    
}