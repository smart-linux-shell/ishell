TARGET := main
SOURCES := screen_ring_buffer.cpp screen.cpp escape.cpp assistant.cpp terminal_multiplexer.cpp main.cpp
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))

INCLUDE := -I.
LIBPATH :=
LIBS := -lncurses -lreadline

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