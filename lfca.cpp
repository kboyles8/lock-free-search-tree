/**
 * The code in this file is from "Lock-free contention adapting search trees",
 * by Kjell Winblad, Konstantinos Sagonas, and Jonsson, Bengt, with slight
 * modification to format and syntax for use with the C++ project.
 *
 * Major modifications:
 * The node struct now inherits route_node (as was likely intended)
 */

#include <atomic>
#include <stack>

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

// Temporarilly stop warnings for treaps. TODO: link our treap
struct treap {
    int a;
};

// Forward declare node for use in structs
struct node;

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
    node *neigh1;                      // First (not joined) neighbor base
    atomic<node *> neigh2{PREPARING};  // Joined n... (neighbor?)
    node *gparent;                     // Grand parent
    node *otherb;                      // Other branch
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

// Forward declare helper functions as needed
treap *all_in_range(lfcat *t, int lo, int hi, rs *help_s);
bool try_replace(lfcat *m, node *b, node *new_b);
void complete_join(lfcat *t, node *m);
node *find_base_stack(node *n, int i, stack<node *> *s);

void low_contention_adaptation(lfcat *t, node *b) { }  // TODO
void high_contention_adaptation(lfcat *m, node *b) { }  // TODO


// This function is undefined in the pdf, assume replaces head of stack with n?
void replace_top(stack<node *> *s, node *n) {
    return;
}

// Also a stub
void copy_state_to(stack<node *> *s, stack<node *> *backup_s) {

    return;
}

// Help functions
bool try_replace(lfcat *m, node *b, node *new_b) {
    node *expectedB = b;

    if (b->parent == nullptr) {
        return m->root.compare_exchange_strong(expectedB, new_b);
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
void help_if_needed(lfcat *t, node *n) {
    if (n->type == join_neighbor) {
        n = n->main_node;
    }

    if (n->type == join_main && n->neigh2.load() == PREPARING) {
        node *expectedNeigh2 = PREPARING;
        n->neigh2.compare_exchange_strong(expectedNeigh2, ABORTED);
    }
    else if (n->type == join_main && n->neigh2.load() > ABORTED) {
        complete_join(t, n);
    }
    else if (n->type == range && n->storage->result.load() == NOT_SET) {
        all_in_range(t, n->lo, n->hi, n->storage);
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

void adapt_if_needed(lfcat *t, node *b) {
    if (!is_replaceable(b)) {
        return;
    }
    else if (new_stat(b, noinfo) > HIGH_CONT) {
        high_contention_adaptation(t, b);
    }
    else if (new_stat(b, noinfo) < LOW_CONT) {
        low_contention_adaptation(t, b);
    }
}

/*
bool do_update(lfcatree *m, treap *(*u)(treap *, int, bool *), int i) {
    contention_info cont_info = uncontened;
    while (true) {
        node *base = find_base_node(m->root.load(), i);
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
*/

// Public interface
/*
bool insert(lfcat *m, int i) {
    return do_update(m, treap_insert, i);
}

bool remove(lfcat *m, int i) {
    return do_update(m, treap_remove, i);
}

bool lookup(lfcat *m, int i) {
    node *base = find_base_node(m->root.load(), i);
    return treap_lookup(base->data, i);
}

void query(lfcat *m, int lo, int hi, void (*trav)(int, void *), void *aux) {
    treap *result = all_in_range(m, lo, hi, nullptr);
    treap_query(result, lo, hi, trav, aux);
}
*/

// Range query helper
node *find_next_base_stack(stack<node *> *s) {
    /*
    node *base = pop(s);
    node *t = top(s);
    if (t == nullptr)
        return nullptr;

    if (t->left.load() == base)
        return leftmost_and_stack(t->right.load(), s);

    int be_greater_than = t->key;
    while (t != nullptr) {
        if (t->valid.load() && t->key > be_greater_than)
            return leftmost_and_stack(t->right.load(), s);
        else {
            pop(s);
            t = top(s);
        }
    }
    */
    return nullptr;
}

node *new_range_base(node *b, int lo, int hi, rs *s) {
    // TODO: implement
    return NULL;
    /*
    return new node{... = b,  // assign fields from b (TODO)
                   lo = lo, hi = hi, storage = s};
    */
}

treap *all_in_range(lfcat *t, int lo, int hi, rs *help_s) {
    stack<node *> *s = new stack<node *>();         // = new_stack();
    stack<node *> *backup_s = new stack<node *>();  // = new_stack();
    stack<node *> *done = new stack<node *>();      // = new_stack();
    node *b;
    rs *my_s;

find_first:
    b = find_base_stack(t->root.load(), lo, s);
    if (help_s != nullptr) {
        if (b->type != range || help_s != b->storage) {
            return help_s->result.load();
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
        done->push(b);
        copy_state_to(s, backup_s);
        // Need to replace these with our treap functions versions of these
        //if (!empty(b->data) && max(b->data) >= hi) {
        //    break;
        //}

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

    // Need to replace treap_join with our treap join, not sure what stack_array represents
    // treap *res = done->stack_array[0]->data;
    for (int i = 1; i < done->size(); i++) {
        // res = treap_join(res, done->stack_array[i]->data);
    }

    // Compare_exchange_strong doesn't work for atomic result and the NOT_SET constant
    // my_s->result is an atomic treap pointer, not_set is a node pointer
    // if (my_s->result.compare_exchange_strong(NOT_SET, res) && done->size > 1) {
    //    astore(&my_s->more_than_one_base, true); original line here, think this is equivalent
    my_s->more_than_one_base.store(true);
    // }

    // adapt_if_needed(t, done->array[r() % done->size]);
    return my_s->result.load();
}

// Contention adaptation
/*
node *secure_join_left(lfcatree *t, node *b) {
    node *n0 = leftmost(b->parent->right.load());
    if (!is_replaceable(n0))
        return nullptr;
    node *m = new node{... = b,  // assign fields from b
                       type = join_main};
    if (!b->parent->left.compare_exchange_strong(b, m))
        return nullptr;
    node *n1 = new node{... = n0,  // assign fields from n0
                        type = join_neighbor, main_node = m};
    if (!try_replace(t, n0, n1))
        goto fail0;
    if (!m->parent->join_id.compare_exchange_strong(nullptr, m))
        goto fail0;
    node *gparent = parent_of(t, m->parent);
    if (gparent == NOT_FOUND ||
        (gparent != nullptr && !gparent->join_id.compare_exchange_strong(nullptr, m)))
        goto fail1;
    m->gparent = gparent;
    m->otherb = m->parent->right.load();
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
*/

void complete_join(lfcat *t, node *m) {
    node *n2 = m->neigh2.load();
    if (n2 == DONE) {
        return;
    }

    try_replace(t, m->neigh1, n2);

    m->parent->valid.store(false);

    node *replacement = m->otherb == m->neigh1 ? n2 : m->otherb;
    if (m->gparent == nullptr) {
        node *expected = m->parent;
        t->root.compare_exchange_strong(expected, replacement);
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

/*
void low_contention_adaptation(lfcatree *t, node *b) {
    if (b->parent == nullptr)
        return;
    if (b->parent->left.load() == b) {
        node *m = secure_join_left(t, b);
        if (m != nullptr)
            complete_join(t, m);
    }
    else if (b->parent->right.load() == b) {
        ...  // Symmetric case
    }
}
*/

/*
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
*/

// Auxilary functions
/*
288 node* find_base_node(node* n, int i) {
289 while(n->type == route){
290 if(i < n->key){
291 n = aload(&n->left);
292 }else{;
293 n = aload(&n->right);
294 }
295 }
296 return n;
297 }
*/

node *find_base_stack(node *n, int i, stack<node *> *s) {
    //  stack_reset(s); I'm not sure what stack_reset means
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

/*
311 node* leftmost_and_stack(node* n, stack* s){
312 while (n->type == route) {
313 push(s, n);
314 n = aload(&n->left);
315 }
316 push(s, n);
317 return n;
318 }
*/

/*
319 node* parent_of(lfcatree* t, node* n){
320 node* prev_node = NULL;
321 node* curr_node = aload(&t->root);
322 while(curr_node != n && curr_node ->type == route){
323 prev_node = curr_node;
324 if(n->key < curr_node ->key){
325 curr_node = aload(&curr_node ->left);
326 }else {
327 curr_node = aload(&curr_node ->right);
328 }
329 }
330 if(curr_node ->type != route){
331 return NOT_FOUND;
332 }
333 return prev_node;
334 }
*/
