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

// Test case: Resizing works as expected when resizing up.
TEST_F(ScreenRingBufferTest, ResizeUpWrapped) {
    screen_ring_buffer->add_char(0, WIDTH - 2, 'a');
    screen_ring_buffer->add_char(0, WIDTH - 1, 'b');
    screen_ring_buffer->add_char(1, 0, 'c');
    screen_ring_buffer->add_char(1, 1, 'd');

    // Resizing up should join
    ScreenRingBuffer new_screen_ring_buffer = ScreenRingBuffer(HEIGHT, WIDTH + 10, 1024, *screen_ring_buffer);

    EXPECT_EQ((new_screen_ring_buffer.get_char(0, WIDTH - 2)), 'a');
    EXPECT_EQ((new_screen_ring_buffer.get_char(0, WIDTH - 1)), 'b');
    EXPECT_EQ((new_screen_ring_buffer.get_char(0, WIDTH)), 'c');
    EXPECT_EQ((new_screen_ring_buffer.get_char(0, WIDTH + 1)), 'd');
};

TEST_F(ScreenRingBufferTest, ResizeUpNotWrapped) {
    screen_ring_buffer->add_char(0, WIDTH - 2, 'a');
    screen_ring_buffer->add_char(0, WIDTH - 1, 'b');
    screen_ring_buffer->new_line(0);
    screen_ring_buffer->add_char(1, 0, 'c');
    screen_ring_buffer->add_char(1, 1, 'd');

    // Resizing up should not join
    ScreenRingBuffer new_screen_ring_buffer = ScreenRingBuffer(HEIGHT, WIDTH + 10, 1024, *screen_ring_buffer);

    EXPECT_EQ((new_screen_ring_buffer.get_char(0, WIDTH - 2)), 'a');
    EXPECT_EQ((new_screen_ring_buffer.get_char(0, WIDTH - 1)), 'b');
    EXPECT_EQ((new_screen_ring_buffer.get_char(0, WIDTH)), 0);
    EXPECT_EQ((new_screen_ring_buffer.get_char(1, 0)), 'c');
    EXPECT_EQ((new_screen_ring_buffer.get_char(1, 1)), 'd');
    EXPECT_EQ((new_screen_ring_buffer.get_char(1, 2)), 0);
};

// Test case: Resizing works as expected when resizing down.
TEST_F(ScreenRingBufferTest, ResizeDownRegular) {
    screen_ring_buffer->add_char(0, WIDTH - 4, 'a');
    screen_ring_buffer->add_char(0, WIDTH - 3, 'b');
    screen_ring_buffer->add_char(0, WIDTH - 2, 'c');
    screen_ring_buffer->add_char(0, WIDTH - 1, 'd');

    // for (int i = 0; i < HEIGHT; i++) {
    //     for (int j = 0; j < WIDTH; j++) {
    //         char ch = screen_ring_buffer->get_char(i, j);
    //         if (ch == 0) {
    //             std::cout << 0;
    //         } else {
    //             std::cout << ch;
    //         }
    //     }

    //     std::cout << "\n";
    // }

    // Resizing down should split
    ScreenRingBuffer new_screen_ring_buffer = ScreenRingBuffer(HEIGHT, WIDTH - 2, 1024, *screen_ring_buffer);

    // for (int i = 0; i < HEIGHT; i++) {
    //     for (int j = 0; j < WIDTH - 2; j++) {
    //         char ch = new_screen_ring_buffer.get_char(i, j);
    //         if (ch == 0) {
    //             std::cout << 0;
    //         } else {
    //             std::cout << ch;
    //         }
    //     }

    //     std::cout << "\n";
    // }

    EXPECT_EQ((new_screen_ring_buffer.get_char(0, WIDTH - 2 - 2)), 'a');
    EXPECT_EQ((new_screen_ring_buffer.get_char(0, WIDTH - 2 - 1)), 'b');
    EXPECT_EQ((new_screen_ring_buffer.get_char(1, 0)), 'c');
    EXPECT_EQ((new_screen_ring_buffer.get_char(1, 1)), 'd');
};

TEST_F(ScreenRingBufferTest, ResizeDownDoubleWrapped) {
    screen_ring_buffer->add_char(0, WIDTH - 4, 'a');
    screen_ring_buffer->add_char(0, WIDTH - 3, 'b');
    screen_ring_buffer->add_char(0, WIDTH - 2, 'c');
    screen_ring_buffer->add_char(0, WIDTH - 1, 'd');
    screen_ring_buffer->add_char(1, WIDTH - 4, 'e');
    screen_ring_buffer->add_char(1, WIDTH - 3, 'f');
    screen_ring_buffer->add_char(1, WIDTH - 2, 'g');
    screen_ring_buffer->add_char(1, WIDTH - 1, 'h');

    ScreenRingBuffer new_screen_ring_buffer = ScreenRingBuffer(HEIGHT, WIDTH - 2, 1024, *screen_ring_buffer);

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH - 2; j++) {
            char ch = new_screen_ring_buffer.get_char(i, j);
            if (i == 0 && j == WIDTH - 2 - 2) {
                EXPECT_EQ(ch, 'a');
            } else if (i == 0 && j == WIDTH - 2 - 1) {
                EXPECT_EQ(ch, 'b');
            } else if (i == 1 && j == 0) {
                EXPECT_EQ(ch, 'c');
            } else if (i == 1 && j == 1) {
                EXPECT_EQ(ch, 'd');
            } else if (i == 2 && j == 0) {
                EXPECT_EQ(ch, 'e');
            } else if (i == 2 && j == 1) {
                EXPECT_EQ(ch, 'f');
            } else if (i == 2 && j == 2) {
                EXPECT_EQ(ch, 'g');
            } else if (i == 2 && j == 3) {
                EXPECT_EQ(ch, 'h');
            } else {
                EXPECT_EQ(ch, 0);
            }
        }
    }
};

TEST_F(ScreenRingBufferTest, ResizeDownDoubleNotWrapped) {
    screen_ring_buffer->add_char(0, WIDTH - 4, 'a');
    screen_ring_buffer->add_char(0, WIDTH - 3, 'b');
    screen_ring_buffer->add_char(0, WIDTH - 2, 'c');
    screen_ring_buffer->add_char(0, WIDTH - 1, 'd');
    screen_ring_buffer->new_line(0);
    screen_ring_buffer->add_char(1, WIDTH - 4, 'e');
    screen_ring_buffer->add_char(1, WIDTH - 3, 'f');
    screen_ring_buffer->add_char(1, WIDTH - 2, 'g');
    screen_ring_buffer->add_char(1, WIDTH - 1, 'h');

    ScreenRingBuffer new_screen_ring_buffer = ScreenRingBuffer(HEIGHT, WIDTH - 2, 1024, *screen_ring_buffer);

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH - 2; j++) {
            char ch = new_screen_ring_buffer.get_char(i, j);
            if (i == 0 && j == WIDTH - 2 - 2) {
                EXPECT_EQ(ch, 'a');
            } else if (i == 0 && j == WIDTH - 2 - 1) {
                EXPECT_EQ(ch, 'b');
            } else if (i == 1 && j == 0) {
                EXPECT_EQ(ch, 'c');
            } else if (i == 1 && j == 1) {
                EXPECT_EQ(ch, 'd');
            } else if (i == 2 && j == WIDTH - 2 - 2) {
                EXPECT_EQ(ch, 'e');
            } else if (i == 2 && j == WIDTH - 2 - 1) {
                EXPECT_EQ(ch, 'f');
            } else if (i == 3 && j == 0) {
                EXPECT_EQ(ch, 'g');
            } else if (i == 3 && j == 1) {
                EXPECT_EQ(ch, 'h');
            } else {
                EXPECT_EQ(ch, 0);
            }
        }
    }
}