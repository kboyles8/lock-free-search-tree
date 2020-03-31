#include <iostream>
#include <random>
#include <chrono>
#include <thread>

#include "lfca.h"

#define NUM_THREADS 4
#define NUM_OPS 40000

#define MAX_TREAPS_NEEDED (2 * NUM_OPS)  // An estimate, about 2 Treaps per operation

using namespace std;
using namespace std::chrono;

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

struct RandomOpVals {
    vector<int> insertVals;
    vector<int> removeVals;
    vector<int> lookupVals;
    vector<int> rangeQueryMinVals;
    vector<int> rangeQueryMaxVals;
    vector<int> randomOps;

    RandomOpVals(int numOps) {
        // Pre-allocate space
        randomOps.reserve(numOps);
        insertVals.reserve(numOps);
        removeVals.reserve(numOps);
        lookupVals.reserve(numOps);
        rangeQueryMinVals.reserve(numOps);
        rangeQueryMaxVals.reserve(numOps);

        // Generate insert and range-query vals, as well as the random operations
        for (int i = 0; i < numOps; i++) {
            insertVals.push_back(valDist(randEngine));

            int rangeQueryA = valDist(randEngine);
            int rangeQueryB = valDist(randEngine);

            // Ensure the smaller random value is placed in the min range query vector
            if (rangeQueryA > rangeQueryB) {
                int temp = rangeQueryA;
                rangeQueryA = rangeQueryB;
                rangeQueryB = temp;
            }

            rangeQueryMinVals.push_back(rangeQueryA);
            rangeQueryMaxVals.push_back(rangeQueryB);

            // Generate random op
            randomOps.push_back(opDist(randEngine));
        }

        // Generate remove and lookup vals based on insert vals
        removeVals = insertVals;
        shuffle(removeVals.begin(), removeVals.end(), randEngine);
        lookupVals = insertVals;
        shuffle(lookupVals.begin(), lookupVals.end(), randEngine);
    }
};

// TODO: handle the possibility of running out of preallocated elements. At the very least, report this in a meaningful way, asking to re-run the program.
static void mixedThread(LfcaTree *tree, int numOps, RandomOpVals *randomOpVals) {
    int op;
    for (int i = 0; i < numOps; i++) {
        op = randomOpVals->randomOps.at(i);

        switch(op) {
            case INSERT:
                tree->insert(randomOpVals->insertVals.at(i));
                break;

            case REMOVE:
                tree->remove(randomOpVals->removeVals.at(i));
                break;

            case LOOKUP:
                tree->lookup(randomOpVals->removeVals.at(i));
                break;

            case RANGE_QUERY:
                tree->rangeQuery(randomOpVals->rangeQueryMinVals.at(i), randomOpVals->rangeQueryMaxVals.at(i));
                break;
        }
    }
}

int main(void) {
    Treap::Preallocate(MAX_TREAPS_NEEDED);

    vector<thread> threads;
    LfcaTree lfcaTree;

    int opsPerThread = NUM_OPS / NUM_THREADS;

    cout << "Generating random values..." << endl;

    vector<RandomOpVals> threadRandomOpVals;

    for (int i = 0; i < NUM_THREADS; i++) {
        threadRandomOpVals.push_back(RandomOpVals(opsPerThread));
    }

    cout << "Running " << NUM_OPS << " random operations total on " << NUM_THREADS << " threads, " << opsPerThread << " operations per thread." << endl;

    high_resolution_clock::time_point start = high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.push_back(thread(mixedThread, &lfcaTree, opsPerThread, &threadRandomOpVals.at(i)));
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.at(i).join();
    }

    // Calculate the total time taken
    high_resolution_clock::time_point end = high_resolution_clock::now();
    duration<double, milli> elapsed = end - start;

    cout << "Finished. (Took " << elapsed.count() << " ms)" << endl;

    Treap::Deallocate();
}