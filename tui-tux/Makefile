TARGET := main
SOURCES := screen_ring_buffer.cpp screen.cpp escape.cpp assistant.cpp assistant_query.cpp bookmarks.cpp terminal_multiplexer.cpp main.cpp agency_request_wrapper.cpp ../https-client/https_client.cpp
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))

INCLUDE := -I.
LIBPATH :=
LIBS := -lncurses -lreadline -lcurl

FLAGS := -Wall
CXXFLAGS := $(FLAGS)

Cxx := g++

all: $(OBJECTS)
	$(Cxx) $(CXXFLAGS) $(INCLUDE) $(OBJECTS) -o $(TARGET) $(LIBPATH) $(LIBS)

%.o: ../src/%.cpp
	$(Cxx) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: clean

clean:
	rm -f *.o
	rm -f $(TARGET)
