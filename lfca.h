#ifndef _LFCA_H
#define _LFCA_H

#include <atomic>
#include <vector>

#include "treap.h"

// Constants
#define NOT_FOUND (node *)1       // Special pointers
#define NOT_SET (vector<int> *)1  // ...
#define PREPARING (node *)0       // Used for join
#define DONE (node *)1            // ...
#define ABORTED (node *)2         // ...

// Contention constants
#define CONT_CONTRIB 250          // For adaptation
#define LOW_CONT_CONTRIB 1        // ...
#define RANGE_QUERY_CONTRIBUTION 100         // ...
#define HIGH_CONTENTION_THRESHOLD 1000            // ...
#define LOW_CONTENTION_THRESHOLD -1000            // ...

enum contention_info {
    contended,
    uncontened,
    noinfo
};

enum node_type {
    route,
    normal,
    join_main,
    join_neighbor,
    range
};

// Forward declare node for use in structs
struct node;
struct rs;

// Data Structures
struct route_node {
    int key{0};                          // Split key
    atomic<node *> left{nullptr};              // < key
    atomic<node *> right{nullptr};             // >= key
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
    node *neigh1 = nullptr;                      // First (not joined) neighbor base
    atomic<node *> neigh2{PREPARING};  // Joined n... (neighbor?)
    node *gparent = nullptr;                     // Grand parent
    node *otherb= nullptr;                      // Other branch

    join_main() : normal_base() { }
    join_main(const join_main &other) : normal_base(other) {
        neigh1 = other.neigh1;
        neigh2.store(other.neigh2.load());
        gparent = other.gparent;
        otherb = other.otherb;
    }
};

struct join_neighbor : virtual normal_base {
    node *main_node = nullptr;  // The main node for the join

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
    int lo = 0;
    int hi = 0;  // Low and high key
    rs *storage = nullptr;

    range_base() : normal_base() { }
    range_base(const range_base &other) : normal_base(other) {
        lo = other.lo;
        hi = other.hi;
        storage = other.storage;  // TODO: should this be copied into a new object instead of linking to the same result storage?
    }
};

struct node : route_node, range_base, join_main, join_neighbor {
    node_type type;

    node() : normal_base(), route_node(), range_base(), join_main(), join_neighbor() { }
    node(const node &other) : normal_base(other), route_node(other), range_base(other), join_main(other), join_neighbor(other) {
        type = other.type;
    }
};

class LfcaTree {
private:
    std::atomic<node *> root{nullptr};

    bool do_update(Treap *(*u)(Treap *, int, bool *), int i);
    std::vector<int> *all_in_range(int lo, int hi, rs *help_s);
    bool try_replace(node *b, node *new_b);
    node *secure_join(node *b, bool left);
    void complete_join(node *m);
    node *parent_of(node *n);
    void adapt_if_needed(node *b);
    void low_contention_adaptation(node *b);
    void high_contention_adaptation(node *b);
    void help_if_needed(node *n);

public:
    LfcaTree();

    bool insert(int val);
    bool remove(int val);
    bool lookup(int val);
    std::vector<int> rangeQuery(int low, int high);
};

#endif /* _LFCA_H */
