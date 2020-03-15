#ifndef _SEARCHTREE_H
#define _SEARCHTREE_H

class SearchTree {
private:
    struct Node {
        bool isRoute {true};
        int val;
        Treap* treap {NULL};

        Node *left {NULL};
        Node *right {NULL};

        Node(int val) {
            this->val = val;
        }
    };

    Node *head {NULL};
public:
    void insert(int val);
    void remove(int val);
    bool lookup(int val);
    vector<int> rangeQuery(int low, int high);
};

#endif /* _SEARCHTREE_H */
