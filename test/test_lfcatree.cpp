#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "../treap.h"
#include "../lfca.h"

#define NUM_THREADS 4
#define PARALLEL_START 0
#define PARALLEL_END 10000

class LfcaTreeTest : public ::testing::Test {
protected:
    LfcaTree lfcaTree;
};

TEST_F(LfcaTreeTest, InsertAndRemoveAndLookup) {
    lfcaTree.insert(1);
    EXPECT_TRUE(lfcaTree.lookup(1));

    lfcaTree.insert(2);
    EXPECT_TRUE(lfcaTree.lookup(2));
    lfcaTree.insert(3);
    EXPECT_TRUE(lfcaTree.lookup(3));
    lfcaTree.insert(4);
    EXPECT_TRUE(lfcaTree.lookup(4));
    lfcaTree.insert(5);
    EXPECT_TRUE(lfcaTree.lookup(5));

    lfcaTree.remove(1);
    EXPECT_FALSE(lfcaTree.lookup(1));
    lfcaTree.remove(2);
    EXPECT_FALSE(lfcaTree.lookup(2));
    lfcaTree.remove(3);
    EXPECT_FALSE(lfcaTree.lookup(3));
    lfcaTree.remove(4);
    EXPECT_FALSE(lfcaTree.lookup(4));
    lfcaTree.remove(5);
    EXPECT_FALSE(lfcaTree.lookup(5));
}

TEST_F(LfcaTreeTest, RangeQuery) {
    for (int i = 1; i <= 9; i++) {
        lfcaTree.insert(i);
    }

    vector<int> expectedQuery = {3, 4, 5, 6, 7, 8, 9};
    vector<int> actualQuery = lfcaTree.rangeQuery(3, 100);
    sort(actualQuery.begin(), actualQuery.end());
    EXPECT_EQ(expectedQuery, actualQuery);

    expectedQuery = {1, 2, 3, 4};
    actualQuery = lfcaTree.rangeQuery(-100, 4);
    sort(actualQuery.begin(), actualQuery.end());
    EXPECT_EQ(expectedQuery, actualQuery);

    expectedQuery = {4, 5, 6};
    actualQuery = lfcaTree.rangeQuery(4, 6);
    sort(actualQuery.begin(), actualQuery.end());
    EXPECT_EQ(expectedQuery, actualQuery);
}

TEST_F(LfcaTreeTest, SplitAndMergeBulkTest) {
    for (int i = 0; i < 1024; i++) {
        lfcaTree.insert(i);
    }

    for (int i = 0; i < 1024; i++) {
        ASSERT_TRUE(lfcaTree.lookup(i));
    }

    for (int i = 0; i < 1024; i++) {
        lfcaTree.remove(i);
        for (int j = i + 1; j < 1024; j++)
        {
            ASSERT_TRUE(lfcaTree.lookup(j));
        }
    }

    for (int i = 0; i < 1024; i++) {
        ASSERT_FALSE(lfcaTree.lookup(i));
    }
}

TEST_F(LfcaTreeTest, RangeQueryBulkTest) {
    for (int i = 0; i < 1024; i++) {
        lfcaTree.insert(i);
    }
    
    vector<int> expectedQuery = {};
    vector<int> actualQuery;
    for (int i = 100; i < 1024; i++) {
        expectedQuery.push_back(i);
        actualQuery = lfcaTree.rangeQuery(100, i);
        sort(actualQuery.begin(), actualQuery.end());
        ASSERT_EQ(expectedQuery, actualQuery);
    }
}

static void insertThread(LfcaTree *tree, int start, int end, int delta) {
    for (int i = start; i <= end; i += delta) {
        tree->insert(i);
    }
}

// A very poor, nondeterministic unit test for crude concurrency. Included just for some sanity, but should not be relied on.
TEST_F(LfcaTreeTest, ParallelInsert) {
    vector<thread> threads;

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.push_back(thread(insertThread, &lfcaTree, PARALLEL_START + i, PARALLEL_END, NUM_THREADS));
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.at(i).join();
    }

    for (int i = 0; i <= PARALLEL_END; i++) {
        ASSERT_TRUE(lfcaTree.lookup(i));
    }
}
