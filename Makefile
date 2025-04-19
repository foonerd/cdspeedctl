# Makefile for cdspeedctl

CC      := gcc
CFLAGS  := -Wall -Wextra -O2
TARGET  := cdspeedctl
SRC     := cdspeedctl.c

PREFIX  ?= /usr

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -D -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: all clean install
