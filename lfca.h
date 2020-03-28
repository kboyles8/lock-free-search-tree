#ifndef _LFCA_H
#define _LFCA_H

#include <atomic>
#include <vector>

#include "treap.h"

// Forward declare node for use in structs
struct node;
struct rs;

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
    LfcaTree() { };

    bool insert(int val);
    bool remove(int val);
    bool lookup(int val);
    std::vector<int> rangeQuery(int low, int high);
};

#endif /* _LFCA_H */
