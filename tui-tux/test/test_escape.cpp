#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <stdlib.h>

#include <escape.hpp>

class EscapeTest : public ::testing::Test {};

// Test case: E_KEY_CLEAR
TEST_F(EscapeTest, CLEAR) {
    std::string s;
    s = "\x1b[J"; EXPECT_EQ(escape(s).ch, E_KEY_CLEAR);
    s = "A\x1b[J"; EXPECT_EQ(escape(s).ch, 0);
    s = "\x1b[JA"; EXPECT_EQ(escape(s).ch, 0);
    s = "\x1b[1J"; EXPECT_EQ(escape(s).ch, 0);
};

// Test case: E_KEY_DCH
TEST_F(EscapeTest, DCH) {
    TerminalChar tch;
    std::string s;

    s = "\x1b[P"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_DCH); EXPECT_EQ(tch.args.size(), 0);
    s = "\x1b[16P"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_DCH); EXPECT_TRUE(tch.args.size() == 1 && tch.args[0] == 16);
    s = "A\x1b[16P"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
};

// Test case: E_KEY_EL
TEST_F(EscapeTest, EL) {
    std::string s;
    s = "\x1b[K"; EXPECT_EQ(escape(s).ch, E_KEY_EL);
    s = "A\x1b[K"; EXPECT_EQ(escape(s).ch, 0);
    s = "\x1b[KA"; EXPECT_EQ(escape(s).ch, 0);
    s = "\x1b[1K"; EXPECT_EQ(escape(s).ch, 0);
};

// Test case: E_KEY_CUP
TEST_F(EscapeTest, CUP) {
    TerminalChar tch;
    std::string s;

    s = "\x1b[H"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUP); EXPECT_EQ(tch.args.size(), 0);
    s = "\x1b[16H"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
    s = "\x1b[16;1H"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUP); EXPECT_TRUE(tch.args.size() == 2 && tch.args[0] == 16 && tch.args[1] == 1);
    s = "\x1b[;16H"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
    s = "A\x1b[16;1H"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
};

// Test case: E_KEY_VPA
TEST_F(EscapeTest, VPA) {
    TerminalChar tch;
    std::string s;

    s = "\x1b[d"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_VPA); EXPECT_EQ(tch.args.size(), 0);
    s = "\x1b[16d"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_VPA); EXPECT_TRUE(tch.args.size() == 1 && tch.args[0] == 16);
    s = "A\x1b[16d"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
};

// Test case: E_KEY_CUB
TEST_F(EscapeTest, CUB) {
    TerminalChar tch;
    std::string s;

    s = "\x1b[D"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUB); EXPECT_EQ(tch.args.size(), 0);
    s = "\x1b[16D"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUB); EXPECT_TRUE(tch.args.size() == 1 && tch.args[0] == 16);
    s = "A\x1b[16D"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
};

// Test case: E_KEY_CUF
TEST_F(EscapeTest, CUF) {
    TerminalChar tch;
    std::string s;

    s = "\x1b[C"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUF); EXPECT_EQ(tch.args.size(), 0);
    s = "\x1b[16C"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUF); EXPECT_TRUE(tch.args.size() == 1 && tch.args[0] == 16);
    s = "A\x1b[16C"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
};

// Test case: E_KEY_CUU
TEST_F(EscapeTest, CUU) {
    TerminalChar tch;
    std::string s;

    s = "\x1b[A"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUU); EXPECT_EQ(tch.args.size(), 0);
    s = "\x1b[16A"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUU); EXPECT_TRUE(tch.args.size() == 1 && tch.args[0] == 16);
    s = "A\x1b[16A"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
};

// Test case: E_KEY_CUD
TEST_F(EscapeTest, CUD) {
    TerminalChar tch;
    std::string s;

    s = "\x1b[B"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUD); EXPECT_EQ(tch.args.size(), 0);
    s = "\x1b[16B"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_CUD); EXPECT_TRUE(tch.args.size() == 1 && tch.args[0] == 16);
    s = "A\x1b[16B"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
};

// Test case: E_KEY_RI
TEST_F(EscapeTest, RI) {
    std::string s;
    s = "\x1bM"; EXPECT_EQ(escape(s).ch, E_KEY_RI);
    s = "A\x1bM"; EXPECT_EQ(escape(s).ch, 0);
    s = "\x1bMA"; EXPECT_EQ(escape(s).ch, 0);
    s = "\x1b\1M"; EXPECT_EQ(escape(s).ch, 0);
};

// Test case: E_KEY_ICH
TEST_F(EscapeTest, ICH) {
    TerminalChar tch;
    std::string s;

    s = "\x1b[@"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_ICH); EXPECT_EQ(tch.args.size(), 0);
    s = "\x1b[16@"; tch = escape(s); EXPECT_EQ(tch.ch, E_KEY_ICH); EXPECT_TRUE(tch.args.size() == 1 && tch.args[0] == 16);
    s = "A\x1b[16@"; tch = escape(s); EXPECT_EQ(tch.ch, 0);
};

// Test case: Invalid escape.
TEST_F(EscapeTest, InvalidEscape) {
    std::string s = "Test invalid";
    TerminalChar tch = escape(s);

    EXPECT_EQ(tch.ch, 0);
    EXPECT_EQ(tch.sequence, s);
};

// Test case: Read and escape works correctly.
TEST_F(EscapeTest, ReadAndEscape) {
    std::string s = "Test\x1b[16;1HTest";
    int fd[2];
    pipe(fd);
    write(fd[1], s.c_str(), s.size());

    std::vector<TerminalChar> vec;

    int n = read_and_escape(fd[0], vec);

    close(fd[0]);
    close(fd[1]);

    EXPECT_TRUE(n == 15 && vec.size() == 9 &&
        vec[0].ch == 'T' && vec[5].ch == 'T' &&
        vec[1].ch == 'e' && vec[6].ch == 'e' &&
        vec[2].ch == 's' && vec[7].ch == 's' &&
        vec[3].ch == 't' && vec[8].ch == 't' &&
        vec[4].ch == E_KEY_CUP && vec[4].args.size() == 2 && vec[4].args[0] == 16 && vec[4].args[1] == 1
    );

};
