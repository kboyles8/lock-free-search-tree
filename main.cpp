#include <iostream>
#include <random>

#include "searchtree.h"

#define NUM_THREADS 4
#define NUM_OPS 10000

using namespace std;

enum Operation {
    INSERT,
    REMOVE,
    LOOKUP,
    RANGE_QUERY,

    // Count the number of operations
    OPS_COUNT
};

static mt19937 randEngine {(unsigned int)time(NULL)};
static uniform_int_distribution<int> valDist {numeric_limits<int>::min(), numeric_limits<int>::max()};
static uniform_int_distribution<int> opDist {0, OPS_COUNT-1};

static void mixedThread(SearchTree *tree, int numOps) {
    int op, val;
    for (int i = 0; i < numOps; i++) {
        op = opDist(randEngine);
        val = valDist(randEngine);

        switch(op) {
            case INSERT:
                tree->insert(val);
                break;

            case REMOVE:
                tree->remove(val);
                break;

            case LOOKUP:
                tree->lookup(val);
                break;

            case RANGE_QUERY:
                int val2 = valDist(randEngine);

                // Ensure val is smaller than val2
                if (val > val2) {
                    int temp = val;
                    val = val2;
                    val2 = temp;
                }

                tree->rangeQuery(val, val2);
                break;
        }
    }
}

int main(void) {
    vector<thread> threads;
    SearchTree searchtree;

    int opsPerThread = NUM_OPS / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.push_back(thread(mixedThread, &searchtree, opsPerThread));
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.at(i).join();
    }
}