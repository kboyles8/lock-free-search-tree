#include <gtest/gtest.h>

#include "../treap.h"

class TreapTest : public ::testing::Test {
protected:
    // void SetUp() override { }
    // void TearDown() override { }

    Treap treap;
};

TEST_F(TreapTest, Insert) {
    EXPECT_EQ(0, treap.getSize());

    treap.sequentialInsert(5);

    EXPECT_EQ(1, treap.getSize());

    treap.sequentialInsert(3);

    EXPECT_EQ(2, treap.getSize());
}