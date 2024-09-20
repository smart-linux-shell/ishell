TARGET := ishell
TEST_TARGET := test_ishell
SOURCES := screen_ring_buffer.cpp screen.cpp escape.cpp assistant_query.cpp bookmarks.cpp assistant.cpp terminal_multiplexer.cpp main.cpp agency_request_wrapper.cpp https_client.cpp
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))

OBJECTS := $(patsubst %, bin/%, $(OBJECTS))
SOURCES := $(patsubst %, src/%, $(SOURCES))

TEST_SOURCES := test/test_bookmarks.cpp
TEST_OBJECTS := $(patsubst test/%.cpp, bin/test/%.o,$(TEST_SOURCES))

INCLUDE := -Iinclude
LIBS := -lncurses -lreadline -lcurl -lgtest -lgmock -lpthread

FLAGS := -Wall
CXXFLAGS := $(FLAGS)

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
test: $(TEST_OBJECTS)
	$(Cxx) $(CXXFLAGS) $(INCLUDE) $(TEST_OBJECTS) src/bookmarks.cpp -o $(TEST_TARGET) $(LIBS)

run_test: test
	./$(TEST_TARGET)

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_OBJECTS) $(TEST_TARGET)
