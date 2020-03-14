#include <gtest/gtest.h>

#include "../treap.h"
#include "../searchtree.cpp"


class SearchTreeTest : public ::testing::Test {
protected:
	SearchTree<int> searchtree;
};

TEST_F(SearchTreeTest, InsertAndRemoveAndLookup) {
	searchtree.insert(1);
    EXPECT_EQ(true, searchtree.lookup(1));
	
	searchtree.insert(2);
    EXPECT_EQ(true, searchtree.lookup(1));
	searchtree.insert(3);
    EXPECT_EQ(true, searchtree.lookup(1));
	searchtree.insert(4);
    EXPECT_EQ(true, searchtree.lookup(1));
	searchtree.insert(5);
    EXPECT_EQ(true, searchtree.lookup(1));

	searchtree.remove(1);
    EXPECT_EQ(false, searchtree.lookup(1));

    EXPECT_EQ(true, searchtree.lookup(2));
	searchtree.remove(1);
    EXPECT_EQ(true, searchtree.lookup(3));
	searchtree.remove(1);
    EXPECT_EQ(true, searchtree.lookup(4));
	searchtree.remove(1);
    EXPECT_EQ(true, searchtree.lookup(5));
	searchtree.remove(1);
    EXPECT_EQ(false, searchtree.lookup(1));
}

TEST_F(SearchTreeTest, RangeQuery) {
	searchtree.insert(1);
	searchtree.insert(2);
	searchtree.insert(3);
	searchtree.insert(4);
	searchtree.insert(5);
	searchtree.insert(6);
	searchtree.insert(7);
	searchtree.insert(8);
	searchtree.insert(9);
	
	
	
	vector<int> test = {3, 4, 5, 6, 7, 8, 9};
	vector<int> test2 = searchtree.rangeQuery(3, 100);
	sort(test2.begin(), test2.end());
	
	EXPECT_EQ(test, test2);
	
	test = {1, 2, 3, 4};
	test2 = searchtree.rangeQuery(-100, 4);
	sort(test2.begin(), test2.end());
	
	EXPECT_EQ(test, test2);
	
	test = {4, 5, 6};
	test2 = searchtree.rangeQuery(4, 6);
	sort(test2.begin(), test2.end());
	
    EXPECT_EQ(test, test2);

}

