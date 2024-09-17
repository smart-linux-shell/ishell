TARGET := ishell
SOURCES := screen_ring_buffer.cpp screen.cpp escape.cpp assistant_query.cpp bookmarks.cpp assistant.cpp terminal_multiplexer.cpp main.cpp agency_request_wrapper.cpp https_client.cpp
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))

OBJECTS := $(patsubst %, bin/%, $(OBJECTS))
SOURCES := $(patsubst %, src/%, $(SOURCES))

INCLUDE := -I.
LIBPATH :=
LIBS := -lncurses -lreadline -lcurl

FLAGS := -Wall
CXXFLAGS := $(FLAGS)

Cxx := g++

PREFIX ?= /usr
BINDIR = $(PREFIX)/bin

all: $(OBJECTS)
	$(Cxx) $(CXXFLAGS) $(INCLUDE) $(OBJECTS) -o $(TARGET) $(LIBPATH) $(LIBS)

bin/%.o: src/%.cpp
	$(Cxx) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: clean
FLAGS := -Wall

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(OBJECTS)
	rm -f $(TARGET)