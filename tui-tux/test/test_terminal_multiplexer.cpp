#include <terminal_multiplexer.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fcntl.h>
#include <unistd.h>

#define FIFO_NAME "/tmp/ishell-testing-fifo"

using namespace testing;

class TerminalMultiplexerTest : public Test {
public:
    int stdin_to_app;
    int child_pid;

    void SetUp() override {
        int pipefd[2];

        // Create pipe for communication with app
        int rc = pipe(pipefd);
        if (rc < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // Fork to run app in a separate process
        int pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Redirect stdout and stderr
            int dev_null = open("/dev/null", O_WRONLY);
            dup2(dev_null, STDOUT_FILENO);
            dup2(dev_null, STDERR_FILENO);

            // Redirect stdin
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);

            // Child runs terminal multiplexer
            TerminalMultiplexer tm;
            tm.run();
            exit(EXIT_SUCCESS);
        }

        // Parent
        child_pid = pid;
        close(pipefd[0]);
        stdin_to_app = pipefd[1];
    }

    void TearDown() override {
        close(stdin_to_app);

        // Kill child if not exited yet
        pid_t result = waitpid(child_pid, NULL, WNOHANG);
        if (result == 0) {
            // Still running
            kill(child_pid, SIGKILL);
            waitpid(child_pid, NULL, 0);
        }
    }
};

// Test case: Check if switching to bash window and basic command work
TEST_F(TerminalMultiplexerTest, SwitchAndBash) {
    // Create named pipe to retrieve results
    int rc = mkfifo(FIFO_NAME, 0666);
    if (rc < 0) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    // Test bash by sending test data to the named pipe
    // First press ^B-TAB to switch to bash, write the command and exit
    std::string command = "\x02\techo -n test >";
    command += FIFO_NAME;
    command += "; exit\n";
    write(stdin_to_app, command.c_str(), command.size());
    
    // Open named pipe in read non blocking mode
    int pipefd = open(FIFO_NAME, O_RDONLY | O_NONBLOCK);

    // Try to get results for 1-2 seconds
    char buf[128] = {0};
    int n;

    time_t time_start = time(NULL);

    while (1) {
        n = read(pipefd, buf, sizeof(buf) - 1);
        if (n > 0) {
            break;
        }

        // Check if time is up
        if (time(NULL) - time_start >= 2) {
            break;
        }
    }

    close(pipefd);

    unlink(FIFO_NAME);

    EXPECT_EQ(n, 4);
    EXPECT_STREQ(buf, "test");
};
