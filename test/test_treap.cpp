#include <gtest/gtest.h>

#include "../treap.h"

class TreapTest : public ::testing::Test {
protected:
    // void SetUp() override { }
    void TearDown() override {
        // Free the left and right tree if needed
        if (left != nullptr) {
            delete(left);
        }
        if (right != nullptr) {
            delete(right);
        }
    }

    Treap treap;
    Treap *left {nullptr};
    Treap *right {nullptr};
};

TEST_F(TreapTest, InsertAndRemove) {
    EXPECT_EQ(0, treap.getSize());

    treap.sequentialInsert(5);
    EXPECT_EQ(1, treap.getSize());

    treap.sequentialInsert(3);
    ASSERT_EQ(2, treap.getSize());

    treap.sequentialRemove(5);
    EXPECT_EQ(1, treap.getSize());

    treap.sequentialRemove(3);
    EXPECT_EQ(0, treap.getSize());
}

TEST_F(TreapTest, Contains) {
    EXPECT_FALSE(treap.contains(1));
    treap.sequentialInsert(1);
    ASSERT_TRUE(treap.contains(1));

    EXPECT_FALSE(treap.contains(2));
    treap.sequentialInsert(2);
    ASSERT_TRUE(treap.contains(2));

    EXPECT_TRUE(treap.contains(1));
    EXPECT_TRUE(treap.sequentialRemove(1));
    ASSERT_FALSE(treap.contains(1));

    EXPECT_TRUE(treap.contains(2));
    EXPECT_TRUE(treap.sequentialRemove(2));
    ASSERT_FALSE(treap.contains(2));
}

TEST_F(TreapTest, RemoveNonExisting) {
    EXPECT_FALSE(treap.contains(1));
    EXPECT_FALSE(treap.contains(2));
    EXPECT_EQ(0, treap.getSize());

    treap.sequentialInsert(1);
    treap.sequentialInsert(2);

    EXPECT_TRUE(treap.contains(1));
    EXPECT_TRUE(treap.contains(2));
    ASSERT_EQ(2, treap.getSize());

    EXPECT_FALSE(treap.sequentialRemove(3));
    EXPECT_EQ(2, treap.getSize());
    EXPECT_TRUE(treap.contains(1));
    EXPECT_TRUE(treap.contains(2));
}

TEST_F(TreapTest, FillingToLimit) {
    ASSERT_EQ(treap.getSize(), 0);

    for (int i = 1; i <= TREAP_NODES; i++) {
        treap.sequentialInsert(i);
        ASSERT_EQ(i, treap.getSize());
        ASSERT_TRUE(treap.contains(i));
    }

    // The treap is now full. New elements can't be added
    bool correctException = false;
    try {
        treap.sequentialInsert(TREAP_NODES + 1);
    } catch (out_of_range e) {
        correctException = true;
    } catch (...) { }

    EXPECT_TRUE(correctException);
    EXPECT_EQ(TREAP_NODES, treap.getSize());
    EXPECT_FALSE(treap.contains(TREAP_NODES + 1));
}

TEST_F(TreapTest, FillingAndEmptying) {
    ASSERT_EQ(treap.getSize(), 0);

    // Fill the treap
    for (int i = 1; i <= TREAP_NODES; i++) {
        treap.sequentialInsert(i);
        ASSERT_EQ(i, treap.getSize());
        ASSERT_TRUE(treap.contains(i));
    }

    // Empty the treap
    for (int i = 1; i <= TREAP_NODES; i++) {
        ASSERT_TRUE(treap.sequentialRemove(i));
        ASSERT_EQ(TREAP_NODES - i, treap.getSize());
        ASSERT_FALSE(treap.contains(i));
    }
}

TEST_F(TreapTest, FullSplit) {
    ASSERT_EQ(treap.getSize(), 0);

    // Fill the treap
    for (int i = 1; i <= TREAP_NODES; i++) {
        treap.sequentialInsert(i);
        ASSERT_EQ(i, treap.getSize());
        ASSERT_TRUE(treap.contains(i));
    }

    // Split the treap
    treap.split(&left, &right);

    // The split is non-deterministic, but there are certain properties that must hold. Test for these

    // Test that all numbers are in one of the two treaps
    for (int i = 1; i <= TREAP_NODES; i++) {
        bool inLeft = left->contains(i);
        bool inRight = right->contains(i);

        // Make sure the number exists
        ASSERT_TRUE(inLeft || inRight);

        // Make sure the number is not in both treaps
        ASSERT_FALSE(inLeft && inRight);
    }

    // Test that all numbers in the left treap are smaller than in the right treap
    int minInt = numeric_limits<int>::min();
    int maxInt = numeric_limits<int>::max();
    int largestInLeft = minInt;
    int smallestInRight = maxInt;
    for (int i = 1; i <= TREAP_NODES; i++) {
        if (left->contains(i) && i > largestInLeft) {
            largestInLeft = i;
        }

        if (right->contains(i) && i < smallestInRight) {
            smallestInRight = i;
        }
    }

    // Sometimes, the treap split can end up with an empty tree on the right side, depending on how the tree is structured. Allow this for now
    EXPECT_LT(largestInLeft, smallestInRight);
}

TEST_F(TreapTest, SplitEmpty) {
    ASSERT_EQ(treap.getSize(), 0);

    // Split the treap
    treap.split(&left, &right);

    ASSERT_EQ(left->getSize(), 0);
    ASSERT_EQ(right->getSize(), 0);
}