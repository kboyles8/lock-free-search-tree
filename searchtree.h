#ifndef _SEARCHTREE_H
#define _SEARCHTREE_H

#include <bitset.h>
#include <mrlock.h>

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

    class ScopedMrLock {
    private:
        uint32_t handle;
        MRLock<Bitset> *lock;

    public:
        ScopedMrLock(MRLock<Bitset> *mrlock, Bitset resources) {
            lock = mrlock;
            handle = mrlock->Lock(resources);
        }

        ~ScopedMrLock() {
            lock->Unlock(handle);
        }
    };

    Node *head {NULL};

    MRLock<Bitset> mrlock;
    Bitset treeLock;

public:
    SearchTree();

    void insert(int val);
    void remove(int val);
    bool lookup(int val);
    vector<int> rangeQuery(int low, int high);
};

#endif /* _SEARCHTREE_H */
