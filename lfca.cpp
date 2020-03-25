/**
 * The code in this file is from "Lock-free contention adapting search trees",
 * by Kjell Winblad, Konstantinos Sagonas, and Jonsson, Bengt, with slight
 * modification to format and syntax for use with the C++ project.
 *
 * Major modifications:
 * The node struct now inherits route_node (as was likely intended)
 */

#include <atomic>

using namespace std;

// Constants
#define CONT_CONTRIB 250     // For adaptation
#define LOW_CONT_CONTRIB 1   // ...
#define RANGE_CONTRIB 100    // ...
#define HIGH_CONT 1000       // ...
#define LOW_CONT -1000       // ...
#define NOT_FOUND (node *)1  // Special pointers
#define NOT_SET (treap *)1   // ...
#define PREPARING (node *)0  // Used for join
#define DONE (node *)1       // ...
#define ABORTED (node *)2    // ...
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
};

struct normal_base {
    treap *data = nullptr;   // Items in the set
    int stat = 0;            // Statistics variable
    node *parent = nullptr;  // Parent node or NULL (root)
};

struct join_main : virtual normal_base {
    node *neigh1;                     // First (not joined) neighbor base
    atomic<node *> neigh2 {PREPARING};  // Joined n...
    node *gparent;                    // Grand parent
    node *otherb;                     // Other branch
};

struct join_neighbor : virtual normal_base {
    node *main_node;  // The main node for the join
};

struct rs {                           // Result storage for range queries
    atomic<treap *> result{NOT_SET};  // The result
    atomic<bool> more_than_one_base{false};
};

struct range_base : virtual normal_base {
    int lo;
    int hi;  // Low and high key
    rs *storage;
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
};

struct lfcat {
    atomic<node *> root;
};

// Help functions
bool try_replace(lfcat *m, node *b, node *new_b) {
    if (b->parent == nullptr)
        return CAS(&m->root, b, new_b);
    else if (aload(&b->parent->left) == b)
        return CAS(&b->parent->left, b, new_b);
    else if (aload(&b->parent->right) == b)
        return CAS(&b->parent->right, b, new_b);
    else
        return false;
}

// TODO: Make this a lot clearer
bool is_replaceable(node *n) {
    return (
        n->type == normal ||
        (n->type == join_main && aload(&n->neigh2) == ABORTED) ||
        (n->type == join_neighbor && (aload(&n->main_node->neigh2) == ABORTED ||
                                      aload(&n->main_node->neigh2) == DONE)) ||
        (n->type == range && aload(&n->storage->result) != NOT_SET)
    );
}

// Help functions
void help_if_needed(lfcatree *t, node *n) {
    if (n->type == join_neighbor)
        n = n->main_node;

    if (n->type == join_main && aload(&n->neigh2) == PREPARING) {
        CAS(&n->neigh2, PREPARING, ABORTED);
    }
    else if (n->type == join_main && aload(&n->neigh2) > ABORTED) {
        complete_join(t, n);
    }
    else if (n->type == range && aload(&n->storage->result) == NOT_SET) {
        all_in_range(t, n->lo, n->hi, n->storage);
    }
}

int new_stat(node *n, contention_info info) {
    int range_sub = 0;
    if (n->type == range && aload(&n->storage->more_than_one_base)) {
        range_sub = RANGE_CONTRIB;
    }

    if (info == contended && n->stat <= HIGH_CONT) {
        return n->stat + CONT_CONTRIB - range_sub;
    }
    else if (info == uncontened && n->stat >= LOW_CONT) {
        return n->stat - LOW_CONT_CONTRIB - range_sub;
    }
    else {
        return n->stat;
    }
}

void adapt_if_needed(lfcatree *t, node *b) {
    if (!is_replaceable(b))
        return;
    else if (new_stat(b, noinfo) > HIGH_CONT)
        high_contention_adaptation(t, b);
    else if (new_stat(b, noinfo) < LOW_CONT)
        low_contention_adaptation(t, b);
}

bool do_update(lfcatree *m, treap *(*u)(treap *, int, bool *), int i) {
    contention_info cont_info = uncontened;
    while (true) {
        node *base = find_base_node(aload(&m->root), i);
        if (is_replaceable(base)) {
            bool res;
            node *newb = new node {
                type = normal, parent = base->parent,
                data = u(base->data, i, &res), stat = new_stat(base, cont_info)
            }
            if (try_replace(m, base, newb)) {
                adapt_if_needed(m, newb);
                return res;
            }
        }
        cont_info = contended;
        help_if_needed(m, base);
    }
}

// Public interface
bool insert(lfcat *m, int i) {
    return do_update(m, treap_insert, i);
}

bool remove(lfcat *m, int i) {
    return do_update(m, treap_remove, i);
}

bool lookup(lfcat *m, int i) {
    node *base = find_base_node(aload(&m->root), i);
    return treap_lookup(base->data, i);
}

void query(lfcat *m, int lo, int hi, void (*trav)(int, void *), void *aux) {
    treap *result = all_in_range(m, lo, hi, nullptr);
    treap_query(result, lo, hi, trav, aux);
}

// Range query helper
node *find_next_base_stack(stack *s) {
    node *base = pop(s);
    node *t = top(s);
    if (t == nullptr)
        return nullptr;

    if (aload(&t->left) == base)
        return leftmost_and_stack(aload(&t->right), s);

    int be_greater_than = t->key;
    while (t != nullptr) {
        if (aload(&t->valid) && t->key > be_greater_than)
            return leftmost_and_stack(aload(&t->right), s);
        else {
            pop(s);
            t = top(s);
        }
    }
    return nullptr;
}

node *new_range_base(node *b, int lo, int hi, rs *s) {
    return new node{... = b,  // assign fields from b (TODO)
                    lo = lo, hi = hi, storage = s};
}

treap *all_in_range(lfcat *t, int lo, int hi, rs *help_s) {
    stack *s = new_stack();
    stack *backup_s = new_stack();
    stack *done = new_stack();
    node *b;
    rs *my_s;

find_first:
    b = find_base_stack(aload(&t->root), lo, s);
    if (help_s != nullptr) {
        if (b->type != range || help_s != b->storage) {
            return aload(&help_s->result);
        }
        else {
            my_s = help_s;
        }
    }
    else if (is_replaceable(b)) {
        my_s = new rs;
        node *n = new_range_base(b, lo, hi, my_s);
        if (!try_replace(t, b, n)) {
            goto find_first;
        }
        replace_top(s, n);
    }
    else if (b->type == range && b->hi >= hi) {
        return all_in_range(t, b->lo, b->hi, b->storage);
    }
    else {
        help_if_needed(t, b);
        goto find_first;
    }

    while (true) {  // Find remaining base nodes
        push(done, b);
        copy_state_to(s, backup_s);
        if (!empty(b->data) && max(b->data) >= hi) {
            break;
        }

    find_next_base_node:
        b = find_next_base_stack(s);
        if (b == nullptr) {
            break;
        }
        else if (aload(&my_s->result) != NOT_SET) {
            return aload(&my_s->result);
        }
        else if (b->type == range && b->storage == my_s) {
            continue;
        }
        else if (is_replaceable(b)) {
            node *n = new_range_base(b, lo, hi, my_s);
            if (try_replace(t, b, n)) {
                replace_top(s, n);
                continue;
            }
            else {
                copy_state_to(backup_s, s);
                goto find_next_base_node;
            }
        }
        else {
            help_if_needed(t, b);
            copy_state_to(backup_s, s);
            goto find_next_base_node;
        }
    }

    treap *res = done->stack_array[0]->data;
    for (int i = 1; i < done->size; i++) {
        res = treap_join(res, done->stack_array[i]->data);
    }

    if (CAS(&my_s->result, NOT_SET, res) && done->size > 1) {
        astore(&my_s->more_than_one_base, true);
    }

    adapt_if_needed(t, done->array[r() % done->size]);
    return aload(&my_s->result);
}

// Contention adaptation
node *secure_join_left(lfcatree *t, node *b) {
    node *n0 = leftmost(aload(&b->parent->right));
    if (!is_replaceable(n0))
        return nullptr;
    node *m = new node{... = b,  // assign fields from b
                       type = join_main};
    if (!CAS(&b->parent->left, b, m))
        return nullptr;
    node *n1 = new node{... = n0,  // assign fields from n0
                        type = join_neighbor, main_node = m};
    if (!try_replace(t, n0, n1))
        goto fail0;
    if (!CAS(&m->parent->join_id, nullptr, m))
        goto fail0;
    node *gparent = parent_of(t, m->parent);
    if (gparent == NOT_FOUND ||
        (gparent != nullptr && !CAS(&gparent->join_id, nullptr, m)))
        goto fail1;
    m->gparent = gparent;
    m->otherb = aload(&m->parent->right);
    m->neigh1 = n1;
    node *joinedp = m->otherb == n1 ? gparent : n1->parent;
    if (CAS(&m->neigh2, PREPARING,
            new node{... = n1,  // assign fields from n1
                     type = join_neighbor, parent = joinedp, main_node = m,
                     data = treap_join(m, n1)}))
        return m;
    if (gparent == nullptr)
        goto fail1;
    astore(&gparent->join_id, nullptr);
fail1:
    astore(&m->parent->join_id, nullptr);
fail0:
    astore(&m->neigh2, ABORTED);
    return nullptr;
}

void complete_join(lfcatree *t, node *m) {
    node *n2 = aload(&m->neigh2);
    if (n2 == DONE)
        return;
    try_replace(t, m->neigh1, n2);
    astore(&m->parent->valid, false);
    node *replacement = m->otherb == m->neigh1 ? n2 : m->otherb;
    if (m->gparent == nullptr) {
        CAS(&t->root, m->parent, replacement);
    }
    else if (aload(&m->gparent->left) == m->parent) {
        CAS(&m->gparent->left, m->parent, replacement);
        CAS(&m->gparent->join_id, m, nullptr);
    }
    else if (aload(&m->gparent->right) == m->parent) {
        ...  // Symmetric case
    }
    astore(&m->neigh2, DONE);
}

void low_contention_adaptation(lfcatree *t, node *b) {
    if (b->parent == nullptr)
        return;
    if (aload(&b->parent->left) == b) {
        node *m = secure_join_left(t, b);
        if (m != nullptr)
            complete_join(t, m);
    }
    else if (aload(&b->parent->right) == b) {
        ...  // Symmetric case
    }
}

void high_contention_adaptation(lfcatree *m, node *b) {
    if (less_than_two_items(b->data))
        return;
    node* r = new node {
        type = route,
        key = split_key(b->data),
        left = new node{type = normal, parent= r, stat= 0,
        data = split_left(b->data)}),
        right = ..., // Symmetric case
        valid = true
    };

    try_replace(m, b, r);
}
