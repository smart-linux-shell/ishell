TARGET := ishell
TEST_TARGET := test_ishell

# Configurable
NO_MAIN_SOURCES := screen.cpp escape.cpp agency_manager.cpp command_manager.cpp bookmark_manager.cpp agent.cpp terminal_multiplexer.cpp agency_request_wrapper.cpp https_client.cpp utils.cpp session_tracker.cpp markdown_manager.cpp
SOURCES := $(NO_MAIN_SOURCES) main.cpp

TEST_SOURCES := test_bookmark_manager.cpp test_agency_request_wrapper.cpp test_https_client.cpp test_escape.cpp test_agency_manager.cpp test_terminal_multiplexer.cpp test_command_manager.cpp
FLAGS := -Wall
LIBS := -lncurses -lreadline -lcurl -lsqlite3
TEST_EXTRA_LIBS := -lgtest -lgmock -L/usr/local/lib -lgtest_main -lpthread

NO_MAIN_OBJECTS := $(patsubst %.cpp,%.o,$(patsubst %, bin/%, $(NO_MAIN_SOURCES)))
OBJECTS := $(patsubst %.cpp,%.o,$(patsubst %, bin/%, $(SOURCES)))
TEST_OBJECTS := $(patsubst %.cpp, %.o, $(patsubst %, bin/test/%, $(TEST_SOURCES)))

SOURCES := $(patsubst %, src/%, $(SOURCES))
NO_MAIN_SOURCES := $(patsubst %, src/%, $(NO_MAIN_SOURCES))
TEST_SOURCES := $(patsubst %, test/%, $(TEST_SOURCES))

INCLUDE := -Iinclude

CXXFLAGS := $(FLAGS) -std=c++17 -g3

Cxx := g++

PREFIX ?= /usr
BINDIR = $(PREFIX)/bin

# Build main project
all: $(OBJECTS)
	$(Cxx) $(CXXFLAGS) $(INCLUDE) $(OBJECTS) -o $(TARGET) $(LIBS)

bin/%.o: src/%.cpp
	$(Cxx) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

# Compile test files
bin/test/%.o: test/%.cpp
	@mkdir -p bin/test
	$(Cxx) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

# Link and build the test executable
test: $(TEST_OBJECTS) $(NO_MAIN_OBJECTS)
	$(Cxx) $(CXXFLAGS) $(INCLUDE) $(TEST_OBJECTS) $(NO_MAIN_OBJECTS) -o $(TEST_TARGET) $(LIBS) $(TEST_EXTRA_LIBS)

run_test: test
	./$(TEST_TARGET)

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_OBJECTS) $(TEST_TARGET)
