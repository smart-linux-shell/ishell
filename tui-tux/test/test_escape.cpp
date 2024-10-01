#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/escape.hpp"

class EscapeTest : public ::testing::Test {};

// Test case: E_KEY_CLEAR
TEST_F(EscapeTest, CLEAR) {
    std::string s;
    s = "\x1b[J"; EXPECT_EQ(escape(s).ch, E_KEY_CLEAR);
    s = "A\x1b[J"; EXPECT_EQ(escape(s).ch, 0);
    s = "\x1b[JA"; EXPECT_EQ(escape(s).ch, 0);
    s = "\x1b[1J"; EXPECT_EQ(escape(s).ch, 0);
};
