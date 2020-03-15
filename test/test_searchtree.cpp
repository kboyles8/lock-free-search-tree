#include <gtest/gtest.h>

#include "../treap.h"
#include "../searchtree.h"


class SearchTreeTest : public ::testing::Test {
protected:
    SearchTree searchtree;
};

TEST_F(SearchTreeTest, InsertAndRemoveAndLookup) {
    searchtree.insert(1);
    EXPECT_TRUE(searchtree.lookup(1));

    searchtree.insert(2);
    EXPECT_TRUE(searchtree.lookup(2));
    searchtree.insert(3);
    EXPECT_TRUE(searchtree.lookup(3));
    searchtree.insert(4);
    EXPECT_TRUE(searchtree.lookup(4));
    searchtree.insert(5);
    EXPECT_TRUE(searchtree.lookup(5));

    searchtree.remove(1);
    EXPECT_FALSE(searchtree.lookup(1));
    searchtree.remove(2);
    EXPECT_FALSE(searchtree.lookup(2));
    searchtree.remove(3);
    EXPECT_FALSE(searchtree.lookup(3));
    searchtree.remove(4);
    EXPECT_FALSE(searchtree.lookup(4));
    searchtree.remove(5);
    EXPECT_FALSE(searchtree.lookup(5));
}

TEST_F(SearchTreeTest, RangeQuery) {
    for (int i = 1; i <= 9; i++) {
        searchtree.insert(i);
    }

    vector<int> expectedQuery = {3, 4, 5, 6, 7, 8, 9};
    vector<int> actualQuery = searchtree.rangeQuery(3, 100);
    sort(actualQuery.begin(), actualQuery.end());
    EXPECT_EQ(expectedQuery, actualQuery);

    expectedQuery = {1, 2, 3, 4};
    actualQuery = searchtree.rangeQuery(-100, 4);
    sort(actualQuery.begin(), actualQuery.end());
    EXPECT_EQ(expectedQuery, actualQuery);

    expectedQuery = {4, 5, 6};
    actualQuery = searchtree.rangeQuery(4, 6);
    sort(actualQuery.begin(), actualQuery.end());
    EXPECT_EQ(expectedQuery, actualQuery);
}

TEST_F(SearchTreeTest, SplitAndMergeBulkTest) {
    for (int i = 0; i < 1024; i++) {
        searchtree.insert(i);
    }

    for (int i = 0; i < 1024; i++) {
        EXPECT_TRUE(searchtree.lookup(i));
    }

    for (int i = 0; i < 1024; i++) {
        searchtree.remove(i);
		for (int j = i + 1; j < 1024; j++)
		{
			EXPECT_TRUE(searchtree.lookup(j));
		}
    }

    for (int i = 0; i < 1024; i++) {
        EXPECT_FALSE(searchtree.lookup(i));
    }
}

TEST_F(SearchTreeTest, RangeQueryBulkTest) {
    for (int i = 0; i < 1024; i++) {
        searchtree.insert(i);
    }
	
    vector<int> expectedQuery = {};
    vector<int> actualQuery;
    for (int i = 100; i < 1024; i++) {
		expectedQuery.push_back(i);
		actualQuery = searchtree.rangeQuery(100, i);
		sort(actualQuery.begin(), actualQuery.end());
		EXPECT_EQ(expectedQuery, actualQuery);
    }

}