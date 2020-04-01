#include <iostream>
#include <random>
#include <chrono>
#include <thread>

#include "lfca.h"
#include "mrlocktree.h"

#define MAX_THREADS 32
#define NUM_OPS 200000

// These values are all estimates, due to the nondeterministic nature of the program
#define MAX_TREAPS_NEEDED (2 * NUM_OPS)
#define MAX_NODES_NEEDED (32 * NUM_OPS)
#define MAX_RESULT_SETS_NEEDED (2 * NUM_OPS)

/**
 * The maximum number of threads that MRLock will support.
 * This is a hard-coded cap in the implementation that cannot be changed.
 */
const int maxMrlockThreads = std::thread::hardware_concurrency();

using namespace std;
using namespace std::chrono;

enum Operation {
    INSERT,
    REMOVE,
    LOOKUP,
    RANGE_QUERY
};

struct OpWeights {
    double insertWeight;
    double removeWeight;
    double lookupWeight;
    double rangeQueryWeight;

    OpWeights(double insertWeight, double removeWeight, double lookupWeight, double rangeQueryWeight) {
        this->insertWeight = insertWeight;
        this->removeWeight = removeWeight;
        this->lookupWeight = lookupWeight;
        this->rangeQueryWeight = rangeQueryWeight;
    }
};

static mt19937 randEngine {(unsigned int)time(NULL)};
static uniform_int_distribution<int> valDist {numeric_limits<int>::min(), numeric_limits<int>::max()};

Operation getRandOp(discrete_distribution<int> opDist) {
    int randNum = opDist(randEngine);
    return (Operation)randNum;
}

struct RandomOpVals {
    vector<int> insertVals;
    vector<int> removeVals;
    vector<int> lookupVals;
    vector<int> rangeQueryMinVals;
    vector<int> rangeQueryMaxVals;
    vector<int> randomOps;

    RandomOpVals(int numOps, OpWeights weights) {
        // Create distribution of operations
        discrete_distribution<int> opDist {weights.insertWeight, weights.removeWeight, weights.lookupWeight, weights.rangeQueryWeight};

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
static void mixedThread(SearchTree *tree, int numOps, RandomOpVals *randomOpVals) {
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

static double RunPerformanceTest(SearchTree *tree, OpWeights weights, int numThreads) {
    vector<thread> threads;

    int opsPerThread = NUM_OPS / numThreads;

    vector<RandomOpVals> threadRandomOpVals;
    for (int i = 0; i < numThreads; i++) {
        threadRandomOpVals.push_back(RandomOpVals(opsPerThread, weights));
    }

    high_resolution_clock::time_point start = high_resolution_clock::now();

    for (int i = 0; i < numThreads; i++) {
        threads.push_back(thread(mixedThread, tree, opsPerThread, &threadRandomOpVals.at(i)));
    }

    for (int i = 0; i < numThreads; i++) {
        threads.at(i).join();
    }

    // Calculate the total time taken
    high_resolution_clock::time_point end = high_resolution_clock::now();
    duration<double, milli> elapsed = end - start;

    return elapsed.count();
}

int main(void) {
    // Set up test weights
    vector<OpWeights> opWeights;
    opWeights.push_back(OpWeights(0.25, 0.25, 0.50, 0.00));  // w:50% r:50%
    opWeights.push_back(OpWeights(0.10, 0.10, 0.80, 0.00));  // w:20% r:80%
    opWeights.push_back(OpWeights(0.005, 0.005, 0.99, 0.00));  // w:1% r:99%

    for (OpWeights weights : opWeights) {
        double lfcaResults[MAX_THREADS];
        double mrlockResults[maxMrlockThreads];

        cout << "Running " << NUM_OPS << " random operations total on 1 to " << MAX_THREADS << " threads. Weights: (insert: "
            << weights.insertWeight << ", remove: " << weights.removeWeight << ", lookup: " << weights.lookupWeight << ", range query: " << weights.rangeQueryWeight << ")..." << endl;

        for (int iThread = 1; iThread <= MAX_THREADS; iThread++) {
            cout << "Running with " << iThread << " thread(s)...";

            Treap::Preallocate(MAX_TREAPS_NEEDED);
            node::Preallocate(MAX_NODES_NEEDED);
            rs::Preallocate(MAX_RESULT_SETS_NEEDED);

            LfcaTree lfcaTree;
            lfcaResults[iThread-1] = RunPerformanceTest(&lfcaTree, weights, iThread);

            Treap::Deallocate();
            node::Deallocate();
            rs::Deallocate();

            // MRLock is internally capped with the number of threads it will allow. Don't exceed this limit, as it causes crashes/hangs
            if (iThread <= maxMrlockThreads) {
                Treap::Preallocate(MAX_TREAPS_NEEDED);

                MrlockTree mrlockTree;
                mrlockResults[iThread-1] = RunPerformanceTest(&mrlockTree, weights, iThread);

                Treap::Deallocate();
            }

            cout << "\r";
        }

        cout << endl;
        cout << "Results (in ms):" << endl;
        cout << "LFCA, ";
        for (int iThread = 0; iThread < MAX_THREADS; iThread++) {
            cout << to_string(lfcaResults[iThread]) << (iThread < MAX_THREADS - 1 ? ", " : "");
        }
        cout << endl;
        cout << "MRLOCK, ";
        for (int iThread = 0; iThread < maxMrlockThreads; iThread++) {
            cout << to_string(mrlockResults[iThread]) << (iThread < maxMrlockThreads - 1 ? ", " : "");
        }
        cout << endl << endl;
    }
}