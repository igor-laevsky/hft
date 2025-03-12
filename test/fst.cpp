#include <gtest/gtest.h>

#include "test.h"

// Function under test
int add(int a, int b) {
    return a + b;
}

// Test case for positive numbers
TEST(AdditionTest, HandlesPositiveNumbers) {
    EXPECT_EQ(add(2, 3), 5);
    EXPECT_EQ(add(10, 20), 20);
}

// Test case for negative numbers
TEST(AdditionTest, HandlesNegativeNumbers) {
  Zoo t;

  EXPECT_EQ(t.add(-2, -3), -5);
  EXPECT_EQ(t.add(-10, 5), -10);
}
