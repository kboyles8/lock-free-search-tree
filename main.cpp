#include <iostream>

#include "treap.h"
#include "searchtree.h"

#define NUM_THREADS 4
#define START 0
#define END 10000

using namespace std;

static void mixedThread(SearchTree *tree, int start, int end) {
    int op, val;
    for (int i = start; i < end; i++)
    {
        op = rand()%4;
        val = rand()%end;
        if (op%4 == 0)
        {
            tree->insert(val);
        }
        else if (op%4 == 1)
        {
            tree->remove(val);
        }
        else if (op%4 == 2)
        {
            tree->lookup(val);
        }
        else
        {
            tree->rangeQuery(0, val);
        }

    }
}

int main(void) {
    srand(time(NULL));
    vector<thread> threads;
    SearchTree searchtree;

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.push_back(thread(mixedThread, &searchtree, START, END));
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.at(i).join();
    }
}