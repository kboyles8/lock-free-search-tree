/**
 * The code in this file is from "Lock-free contention adapting search trees",
 * by Kjell Winblad, Konstantinos Sagonas, and Jonsson, Bengt, with slight
 * modification to format and syntax for use with the C++ project.
 *
 * Major modifications:
 * The node struct now inherits route_node (as was likely intended)
 * C utilities used in the original implementation, such as stack, now use C++ standard library variants
 * Range query results are stored in vectors instead of treaps
 * Our custom immutable treaps are used in place of the original
 */

#include "lfca.h"

#include <stack>

using namespace std;

// Constants
#define CONT_CONTRIB 250          // For adaptation
#define LOW_CONT_CONTRIB 1        // ...
#define RANGE_CONTRIB 100         // ...
#define HIGH_CONT 1000            // ...
#define LOW_CONT -1000            // ...
#define NOT_FOUND (node *)1       // Special pointers
#define NOT_SET (vector<int> *)1  // ...
#define PREPARING (node *)0       // Used for join
#define DONE (node *)1            // ...
#define ABORTED (node *)2         // ...
const int TreapSplitThreshold = 64;
enum contention_info {
    contended,
    uncontened,
    noinfo
};

// Data Structures
struct route_node {
    int key;                          // Split key
    atomic<node *> left;              // < key
    atomic<node *> right;             // >= key
    atomic<bool> valid{true};         // Used for join
    atomic<node *> join_id{nullptr};  // ...

    route_node() { }
    route_node(const route_node &other) {
        key = other.key;
        left.store(other.left.load());
        right.store(other.right.load());
        valid.store(other.valid.load());
        join_id.store(other.join_id.load());
    }
};

struct normal_base {
    Treap *data = nullptr;   // Items in the set
    int stat = 0;            // Statistics variable
    node *parent = nullptr;  // Parent node or NULL (root)

    normal_base() { }
    normal_base(const normal_base &other) {
        data = other.data;
        stat = other.stat;
        parent = other.parent;
    }
};

struct join_main : virtual normal_base {
    node *neigh1;                      // First (not joined) neighbor base
    atomic<node *> neigh2{PREPARING};  // Joined n... (neighbor?)
    node *gparent;                     // Grand parent
    node *otherb;                      // Other branch

    join_main() : normal_base() { }
    join_main(const join_main &other) : normal_base(other) {
        neigh1 = other.neigh1;
        neigh2.store(other.neigh2.load());
        gparent = other.gparent;
        otherb = other.otherb;
    }
};

struct join_neighbor : virtual normal_base {
    node *main_node;  // The main node for the join

    join_neighbor() : normal_base() { }
    join_neighbor(const join_neighbor &other) : normal_base(other) {
        main_node = other.main_node;
    }
};

struct rs {                                 // Result storage for range queries
    atomic<vector<int> *> result{NOT_SET};  // The result
    atomic<bool> more_than_one_base{false};

    rs() { }
    rs(const rs &other) {
        result.store(other.result.load());
        more_than_one_base.store(other.more_than_one_base.load());
    }
};

struct range_base : virtual normal_base {
    int lo;
    int hi;  // Low and high key
    rs *storage;

    range_base() : normal_base() { }
    range_base(const range_base &other) : normal_base(other) {
        lo = other.lo;
        hi = other.hi;
        storage = other.storage;  // TODO: should this be copied into a new object instead of linking to the same result storage?
    }
};

enum node_type {
    route,
    normal,
    join_main,
    join_neighbor,
    range
};

struct node : route_node, range_base, join_main, join_neighbor {
    node_type type;

    node() : normal_base(), route_node(), range_base(), join_main(), join_neighbor() { }
    node(const node &other) : normal_base(other), route_node(other), range_base(other), join_main(other), join_neighbor(other) {
        type = other.type;
    }
};

// Forward declare helper functions as needed
node *find_base_stack(node *n, int i, stack<node *> *s);
node *find_base_node(node *n, int i);
node *leftmost_and_stack(node *n, stack<node *> *s);

// Helper functions for do_update
Treap *treap_insert(Treap *treap, int val, bool *result) {
    Treap *newTreap = treap->immutableInsert(val);
    *result = true;  // Inserts always succeed
    return newTreap;
}

Treap *treap_remove(Treap *treap, int val, bool *result) {
    Treap *newTreap = treap->immutableRemove(val);
    *result = true;  // TODO: Treaps do not currently report success/failure
    return newTreap;
}

// Undefined functions that need implementations:

// This function is undefined in the pdf, assume replaces head of stack with n?
void replace_top(stack<node *> *s, node *n) {
    s->pop();
    s->push(n);
    return;
}

// Assuming this finds the leftmost node for a given node (follow left pointer until the end)
node *leftmost(node *n) {
    node *temp = n;
    while (temp->left != nullptr)
        temp = temp->left;
    return temp;
}

// Opposite version of leftmost for secure_join_right
node *rightmost(node *n) {
    node *temp = n;
    while (temp->right != nullptr)
        temp = temp->right;
    return temp;
}

// Help functions
bool LfcaTree::try_replace(node *b, node *new_b) {
    node *expectedB = b;

    if (b->parent == nullptr) {
        return root.compare_exchange_strong(expectedB, new_b);
    }
    else if (b->parent->left.load() == b) {
        return b->parent->left.compare_exchange_strong(expectedB, new_b);
    }
    else if (b->parent->right.load() == b) {
        return b->parent->right.compare_exchange_strong(expectedB, new_b);
    }

    return false;
}

bool is_replaceable(node *n) {
    switch (n->type) {
        case normal:
            return true;

        case join_main:
            return n->neigh2.load() == ABORTED;

        case join_neighbor: {
            node *neigh2local = n->main_node->neigh2.load();
            return (neigh2local == ABORTED || neigh2local == DONE);
        }

        case range:
            return n->storage->result.load() != NOT_SET;

        default:
            return false;
    }
}

// Help functions
void LfcaTree::help_if_needed(node *n) {
    if (n->type == join_neighbor) {
        n = n->main_node;
    }

    if (n->type == join_main && n->neigh2.load() == PREPARING) {
        node *expectedNeigh2 = PREPARING;
        n->neigh2.compare_exchange_strong(expectedNeigh2, ABORTED);
    }
    else if (n->type == join_main && n->neigh2.load() > ABORTED) {
        complete_join(n);
    }
    else if (n->type == range && n->storage->result.load() == NOT_SET) {
        all_in_range(n->lo, n->hi, n->storage);
    }
}

int new_stat(node *n, contention_info info) {
    int range_sub = 0;
    if (n->type == range && n->storage->more_than_one_base.load()) {
        range_sub = RANGE_CONTRIB;
    }

    if (info == contended && n->stat <= HIGH_CONT) {
        return n->stat + CONT_CONTRIB - range_sub;
    }

    if (info == uncontened && n->stat >= LOW_CONT) {
        return n->stat - LOW_CONT_CONTRIB - range_sub;
    }

    return n->stat;
}

void LfcaTree::adapt_if_needed(node *b) {
    if (!is_replaceable(b)) {
        return;
    }
    else if (new_stat(b, noinfo) > HIGH_CONT || (b->type == normal && b->data->getSize() >= TreapSplitThreshold) ) {
        high_contention_adaptation(b);
    }
    else if (new_stat(b, noinfo) < LOW_CONT) {
        low_contention_adaptation(b);
    }
}

bool LfcaTree::do_update(Treap *(*u)(Treap *, int, bool *), int i) {
    contention_info cont_info = uncontened;

    while (true) {
        node *base = find_base_node(root.load(), i);
        if (is_replaceable(base)) {
            bool res;

            node *newb = new node();
            newb->type = normal;
            newb->parent = base->parent;
            newb->data = u(base->data, i, &res);
            newb->stat = new_stat(base, cont_info);

            if (try_replace(base, newb)) {
                adapt_if_needed(newb);
                return res;
            }
        }

        cont_info = contended;
        help_if_needed(base);
    }
}

// Public interface
LfcaTree::LfcaTree() {
    // Create root node
    node *rootNode = new node();
    rootNode->type = normal;
    rootNode->data =  new Treap();
    root.store(rootNode);
}

bool LfcaTree::insert(int i) {
    return do_update(treap_insert, i);
}

bool LfcaTree::remove(int i) {
    return do_update(treap_remove, i);
}

bool LfcaTree::lookup(int i) {
    node *base = find_base_node(root.load(), i);
    return base->data->contains(i);
}

vector<int> LfcaTree::rangeQuery(int lo, int hi) {
    vector<int> *result = all_in_range(lo, hi, nullptr);
    return *result;  // TODO: memory leak
}

// Range query helper
node *find_next_base_stack(stack<node *> *s) {
    node *base = s->top();
    s->pop();

    if (s->empty()) {
        return nullptr;
    }
    
    node *t = s->top();

    if (t == nullptr) {
        return nullptr;
    }

    if (t->left.load() == base) {
        return leftmost_and_stack(t->right.load(), s);
    }

    int be_greater_than = t->key;
    while (t != nullptr) {
        if (t->valid.load() && (t->key > be_greater_than)) {
            return leftmost_and_stack(t->right.load(), s);
        }
        else {
            s->pop();
            if (s->empty()) {
                return nullptr;
            }
            t = s->top();
        }
    }

    return nullptr;
}

node *new_range_base(node *b, int lo, int hi, rs *s) {
    // Copy the other node
    node *new_base = new node(*b);

    // Set fields
    new_base->lo = lo;
    new_base->hi = hi;
    new_base->storage = s;

    return new_base;
}

vector<int> *LfcaTree::all_in_range(int lo, int hi, rs *help_s) {
    stack<node *> *s = new stack<node *>();
    stack<node *> *backup_s = new stack<node *>();
    vector<node *> *done = new vector<node *>();
    node *b;
    rs *my_s;

find_first:
    b = find_base_stack(root.load(), lo, s);
    if (help_s != nullptr) {
        if (b->type != range || help_s != b->storage) {
            return help_s->result.load();
        }
        else {
            my_s = help_s;
        }
    }
    else if (is_replaceable(b)) {
        my_s = new rs();
        node *n = new_range_base(b, lo, hi, my_s);

        if (!try_replace(b, n)) {
            goto find_first;
        }

        replace_top(s, n);
    }
    else if (b->type == range && b->hi >= hi) {
        return all_in_range(b->lo, b->hi, b->storage);
    }
    else {
        help_if_needed(b);
        goto find_first;
    }

    while (true) {  // Find remaining base nodes
        done->push_back(b);
        *backup_s = *s;  // Backup the result set
        // TODO: replace with our treap functions
        /*
        if (!empty(b->data) && max(b->data) >= hi) {
           break;
        }
        */

    find_next_base_node:
        b = find_next_base_stack(s);
        if (b == nullptr) {
            break;
        }
        else if (my_s->result.load() != NOT_SET) {
            return my_s->result.load();
        }
        else if (b->type == range && b->storage == my_s) {
            continue;
        }
        else if (is_replaceable(b)) {
            node *n = new_range_base(b, lo, hi, my_s);

            if (try_replace(b, n)) {
                replace_top(s, n);
                continue;
            }
            else {
                *s = *backup_s;  // Restore the result set from backup
                goto find_next_base_node;
            }
        }
        else {
            help_if_needed(b);
            *s = *backup_s;  // Restore the result set from backup
            goto find_next_base_node;
        }
    }

    // TODO: replace with our treap functions
    // TODO: `stack_array` likely refers to the internal array that was used to store the struct. This is either the top of the stack, or the bottom, depending on how it was implemented. Verify this and replicate the logic.
    vector<int> *res = new vector<int>(done->front()->data->rangeQuery(lo, hi));  // done->stack_array[0]->data;
    for (size_t i = 1; i < done->size(); i++) {
        vector<int> resTemp = done->at(i)->data->rangeQuery(lo, hi);  // res = treap_join(res, done->stack_array[i]->data);
        res->insert(end(*res), begin(resTemp), end(resTemp));
    }

    vector<int> *expectedResult = NOT_SET;
    if (my_s->result.compare_exchange_strong(expectedResult, res) && done->size() > 1) {
        my_s->more_than_one_base.store(true);
    }

    // TODO: Figure out what is going on here. r() is likely a globally defined random function, picking a random array index.
    // adapt_if_needed(t, done->array[r() % done->size]);
    return my_s->result.load();
}

// Contention adaptation
node *LfcaTree::secure_join(node *b, bool left) {
    node *n0;
    if (left) {
        n0 = leftmost(b->parent->right.load());
    }
    else {
        n0 = rightmost(b->parent->left.load());
    }

    if (!is_replaceable(n0)) {
        return nullptr;
    }

    node *m = new node(*b);  // Copy b
    m->type = join_main;

    node *expectedNode = b;
    if (left) {
        if (!b->parent->left.compare_exchange_strong(expectedNode, m)) {
            return nullptr;
        }
    }
    else {
        if (!b->parent->right.compare_exchange_strong(expectedNode, m)) {
            return nullptr;
        }
    }

    node *n1 = new node(*n0);  // Copy n0
    n1->type = join_neighbor;
    n1->main_node = m;

    if (!try_replace(n0, n1)) {
        m->neigh2.store(ABORTED);
        return nullptr;
    }

    expectedNode = nullptr;
    if (!m->parent->join_id.compare_exchange_strong(expectedNode, m)) {
        m->neigh2.store(ABORTED);
        return nullptr;
    }

    node *gparent = parent_of(m->parent);
    expectedNode = nullptr;
    if (gparent == NOT_FOUND || (gparent != nullptr && !gparent->join_id.compare_exchange_strong(expectedNode, m))) {
        m->parent->join_id.store(nullptr);
        m->neigh2.store(ABORTED);
        return nullptr;
    }

    m->gparent = gparent;
    if (left) {
        m->otherb = m->parent->right.load();
    }
    else {
        m->otherb = m->parent->left.load();
    }
    m->neigh1 = n1;

    node *joinedp = m->otherb == n1 ? gparent : n1->parent;
    node *newNeigh2 = new node(*n1);  // Copy n1
    newNeigh2->type = join_neighbor;
    newNeigh2->parent = joinedp;
    newNeigh2->main_node = m;
    newNeigh2->data = Treap::merge(m->data, n1->data);  // TODO: Verify that all elements in m are less than those in n1

    expectedNode = PREPARING;
    if (m->neigh2.compare_exchange_strong(expectedNode, newNeigh2)) {
        return m;
    }

    if (gparent != nullptr) {
        gparent->join_id.store(nullptr);
    }

    m->parent->join_id.store(nullptr);
    m->neigh2.store(ABORTED);
    return nullptr;
}

void LfcaTree::complete_join(node *m) {
    node *n2 = m->neigh2.load();
    if (n2 == DONE) {
        return;
    }

    try_replace(m->neigh1, n2);

    m->parent->valid.store(false);

    node *replacement = m->otherb == m->neigh1 ? n2 : m->otherb;
    if (m->gparent == nullptr) {
        node *expected = m->parent;
        root.compare_exchange_strong(expected, replacement);
    }
    else if (m->gparent->left.load() == m->parent) {
        node *expected = m->parent;
        m->gparent->left.compare_exchange_strong(expected, replacement);

        expected = m;
        m->gparent->join_id.compare_exchange_strong(expected, nullptr);
    }
    else if (m->gparent->right.load() == m->parent) {
        node *expected = m->parent;
        m->gparent->right.compare_exchange_strong(expected, replacement);

        expected = m;
        m->gparent->join_id.compare_exchange_strong(expected, nullptr);
    }

    m->neigh2.store(DONE);
}

void LfcaTree::low_contention_adaptation(node *b) {
    if (b->parent == nullptr) {
        return;
    }

    if (b->parent->left.load() == b) {
        node *m = secure_join(b, true);
        if (m != nullptr) {
            complete_join(m);
        }
    }
    else if (b->parent->right.load() == b) {
        // TODO: Verify that this "symmetric case" is correct
        node *m = secure_join(b, false);
        if (m != nullptr) {
            complete_join(m);
        }
    }
}

void LfcaTree::high_contention_adaptation(node *b) {
    // Don't split treaps that have too few items
    if (b->data->getSize() < 2) {
        return;
    }

    // Create new route node
    node *r = new node();
    r->type = route;
    r->valid = true;

    // Split the treap
    Treap *leftTreap;
    Treap *rightTreap;
    int splitVal = b->data->split(&leftTreap, &rightTreap);

    // Create left base node
    node *leftNode = new node();
    leftNode->type = normal;
    leftNode->parent = r;
    leftNode->stat = 0;
    leftNode->data = leftTreap;

    // Create right base node
    node *rightNode = new node();
    rightNode->type = normal;
    rightNode->parent = r;
    rightNode->stat = 0;
    rightNode->data = rightTreap;

    // Add the treaps to the route node
    r->key = splitVal;
    r->left = leftNode;
    r->right = rightNode;

    try_replace(b, r);
}

// Auxilary functions
node *find_base_node(node *n, int i) {
    while (n->type == route) {
        if (i < n->key) {
            n = n->left.load();
        }
        else {
            n = n->right.load();
        }
    }

    return n;
}

node *find_base_stack(node *n, int i, stack<node *> *s) {
    // TODO: determine what stack_reset does. (It may just empty the stack)
    //  stack_reset(s);

    while (n->type == route) {
        s->push(n);

        if (i < n->key) {
            n = n->left.load();
        }
        else {
            n = n->right.load();
        }
    }

    s->push(n);
    return n;
}

node *leftmost_and_stack(node *n, stack<node *> *s) {
    while (n->type == route) {
        s->push(n);
        n = n->left.load();
    }

    s->push(n);
    return n;
}

node *LfcaTree::parent_of(node *n) {
    node *prev_node = NULL;
    node *curr_node = root.load();

    while (curr_node != n && curr_node->type == route) {
        prev_node = curr_node;
        if (n->key < curr_node->key) {
            curr_node = curr_node->left.load();
        }
        else {
            curr_node = curr_node->right.load();
        }
    }

    // TODO: This doesn't make sense, and restricts the function to only finding route nodes. It should check if `curr_node` is not `n`.
    if (curr_node->type != route) {
        return NOT_FOUND;
    }

    return prev_node;
}
