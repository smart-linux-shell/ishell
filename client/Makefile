PREFIX ?= /usr
BINDIR = $(PREFIX)/bin

SRC = src/main.c

TARGET = ishell

CC = gcc
CFLAGS = -Wall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)
	
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	
clean:
	rm -f $(TARGET)
