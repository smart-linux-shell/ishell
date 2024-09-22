#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <time.h>
#include <stdlib.h>

#include <iostream>

#include "../include/screen_ring_buffer.hpp"

#define WIDTH 80
#define HEIGHT 24
#define RING_BUFFER_SIZE 1024

class ScreenRingBufferTest : public ::testing::Test {
protected:
    ScreenRingBuffer *screen_ring_buffer;

    void SetUp() override {
        screen_ring_buffer = new ScreenRingBuffer(HEIGHT, WIDTH, RING_BUFFER_SIZE);
    }

    void TearDown() override {
        delete screen_ring_buffer;
    }
};

// Test case: Returns correct result when writing in bounds.
TEST_F(ScreenRingBufferTest, WriteInBoundsTest) {
    // Act
    int result = screen_ring_buffer->add_char(0, 0, 'a');

    // Assert
    EXPECT_EQ(result, 0);
}

// Test case: Returns correct result when writing out of bounds.
TEST_F(ScreenRingBufferTest, WriteOutOfBoundsTest) {
    // Act
    int result = screen_ring_buffer->add_char(24, 0, 'a');

    // Assert
    EXPECT_EQ(result, -1);
}

// Test case: Character gets placed properly when expanding is not needed.
TEST_F(ScreenRingBufferTest, GetCharNoExpansion) {
    // Act
    screen_ring_buffer->add_char(0, 65, 'a');
    char result = screen_ring_buffer->get_char(0, 65);

    // Assert
    EXPECT_EQ(result, 'a');
}

// Test case: Character gets placed properly when expanding is needed.
TEST_F(ScreenRingBufferTest, GetCharExpansion) {
    // Act
    screen_ring_buffer->add_char(22, 77, 'a');
    char result = screen_ring_buffer->get_char(22, 77);

    // Assert
    EXPECT_EQ(result, 'a');
}

// Test case: Scrolling down works correctly.
TEST_F(ScreenRingBufferTest, ScrollDown) {
    // Act
    screen_ring_buffer->add_char(23, 78, 'a');
    screen_ring_buffer->scroll_down();

    char result1 = screen_ring_buffer->get_char(22, 78);
    char result2 = screen_ring_buffer->get_char(23, 78);

    // Assert
    EXPECT_EQ(result1, 'a');
    EXPECT_EQ(result2, 0);
}

// Test case: Scrolling up works correctly.
TEST_F(ScreenRingBufferTest, ScrollUp) {
    // Act
    screen_ring_buffer->add_char(22, 62, 'a');
    screen_ring_buffer->scroll_up();

    char result1 = screen_ring_buffer->get_char(23, 62);
    char result2 = screen_ring_buffer->get_char(22, 62);

    // Assert
    EXPECT_EQ(result1, 'a');
    EXPECT_EQ(result2, 0);
}

// Test case: New line works correctly.
TEST_F(ScreenRingBufferTest, NewLine) {
    // No newlines
    for (int i = 0; i < HEIGHT; i++) {
        bool result = screen_ring_buffer->has_new_line(i);
        EXPECT_EQ(result, false);
    }

    screen_ring_buffer->add_char(0, 0, 'a');
    screen_ring_buffer->new_line(0);

    // New paragraph only after line 0
    for (int i = 0; i < HEIGHT; i++) {
        bool expected = false;

        if (i == 0) {
            expected = true;    
        }

        bool result = screen_ring_buffer->has_new_line(i);
        EXPECT_EQ(result, expected);
    }
}

// Test case: has_new_line returns correct result when out of bounds.
TEST_F(ScreenRingBufferTest, HasNewLineOutOfBounds) {
    bool result1 = screen_ring_buffer->has_new_line(10);
    bool result2 = screen_ring_buffer->has_new_line(HEIGHT + 1);

    ASSERT_EQ(result1, false);
    ASSERT_EQ(result2, false);
}

// Test case: Clear works correctly.
TEST_F(ScreenRingBufferTest, Clear) {
    srand(time(NULL));

    // Adding random characters
    for (int i = 0; i < 100; i++) {
        screen_ring_buffer->add_char(rand() % HEIGHT, rand() % WIDTH, 'a');
    }

    // Clear
    screen_ring_buffer->clear();

    // Assert
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            char result = screen_ring_buffer->get_char(i, j);
            EXPECT_EQ(result, 0);
        }
    }
}

// Test case: Push right works correctly when scrolling is not needed.
TEST_F(ScreenRingBufferTest, PushRightNoScroll) {
    for (int i = 0; i < WIDTH - 1; i++) {
        screen_ring_buffer->add_char(0, i, 'a' + i);
    }

    screen_ring_buffer->push_right(0, 0);

    char result = screen_ring_buffer->get_char(0, 0);
    char expected = 'a';
    EXPECT_EQ(result, expected);

    for (int i = 1; i < WIDTH; i++) {
        result = screen_ring_buffer->get_char(0, i);
        expected = 'a' + i - 1;
        EXPECT_EQ(result, expected);
    }
}

// Test case: Push right works correctly when scrolling is needed.
TEST_F(ScreenRingBufferTest, PushRightScroll) {
    for (int i = 0; i < WIDTH; i++) {
        screen_ring_buffer->add_char(HEIGHT - 1, i, 'a' + i);
    }

    screen_ring_buffer->push_right(HEIGHT - 1, 0);

    // Should've scrolled automatically
    char result = screen_ring_buffer->get_char(HEIGHT - 2, 0);
    char expected = 'a';
    EXPECT_EQ(result, expected);

    for (int i = 1; i < WIDTH; i++) {
        result = screen_ring_buffer->get_char(HEIGHT - 2, i);
        expected = 'a' + i - 1;
        EXPECT_EQ(result, expected);
    }

    // Last line (scrolled)
    result = screen_ring_buffer->get_char(HEIGHT - 1, 0);
    expected = 'a' + WIDTH - 1;
    EXPECT_EQ(result, expected);

    // All the other characters should be empty
    expected = 0;

    for (int i = 1; i < WIDTH; i++) {
        result = screen_ring_buffer->get_char(HEIGHT - 1, i);
        EXPECT_EQ(result, expected);
    }
}

