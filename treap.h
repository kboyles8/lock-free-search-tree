#ifndef TREAP_H
#define TREAP_H

#include <algorithm>
#include <iterator>
#include <limits>
#include <random>
#include <ctime>

using namespace std;

#define TREAP_NODES 64

typedef int TreapIndex;
const TreapIndex NullNode = -1;
const TreapIndex ControlNode = TREAP_NODES;  // The extra node allocated beyond the size of the treap

const int NegInfinity = numeric_limits<int>::min();
const int PosInfinity = numeric_limits<int>::max();

// Random generator
extern thread_local mt19937 randEngine;
extern uniform_int_distribution<int> weightDist;

class Treap {
private:
    struct TreapNode {
        int val;
        int weight;

        TreapIndex parent {NullNode};
        TreapIndex left {NullNode};
        TreapIndex right {NullNode};
    };

    struct TreapTransferInfo {
        bool isLeftChild;
        TreapIndex newParentIndex;
        TreapIndex originalIndex;
    };

    int size {0};
    TreapNode nodes[TREAP_NODES + 1];
    TreapIndex root {NullNode};

    Treap(const Treap &other);

    void moveNode(TreapIndex srcIndex, TreapIndex dstIndex);

    TreapIndex createNewNode(int val);
    TreapIndex transferNodesFrom(Treap *other, TreapIndex rootIndex);

    void bstInsert(TreapIndex index);
    TreapIndex bstFind(int val);

    void leftRotate(TreapIndex index);
    void rightRotate(TreapIndex index);

    void moveUp(TreapIndex index);
    void moveDown(TreapIndex index);

    void insert(int val);
    bool remove(int val);

public:
    Treap();

    Treap *immutableInsert(int val);
    Treap *immutableRemove(int val);

    bool contains(int val);

    int getSize();

    static Treap *merge(Treap *left, Treap *right);
    void split(Treap **left, Treap **right);

    void sequentialInsert(int val);
    bool sequentialRemove(int val);
};

#endif /* TREAP_H */
