TARGET := main
SOURCES := screen_ring_buffer.cpp screen.cpp escape.cpp assistant.cpp terminal_multiplexer.cpp main.cpp agency_request_wrapper.cpp
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))

OBJECTS := $(patsubst %, bin/%, $(OBJECTS))
SOURCES := $(patsubst %, src/%, $(SOURCES))

EXT_SOURCES := ../https-client/https_client.cpp
EXT_OBJECTS := $(patsubst %.cpp, %.o, $(EXT_SOURCES))

INCLUDE := -I.
LIBPATH :=
LIBS := -lncurses -lreadline -lcurl

FLAGS := -Wall
CXXFLAGS := $(FLAGS)

Cxx := g++

all: $(OBJECTS)
	$(Cxx) $(CXXFLAGS) $(INCLUDE) $(OBJECTS) $(EXT_OBJECTS) -o $(TARGET) $(LIBPATH) $(LIBS)

bin/%.o: src/%.cpp
	$(Cxx) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.cpp
	$(Cxx) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: clean
FLAGS := -Wall

clean:
	rm -f $(OBJECTS)
	rm -f $(EXT_OBJECTS)
	rm -f $(TARGET)