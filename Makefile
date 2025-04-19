# Makefile for cdspeedctl

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2
TARGET  := cdspeedctl
SRC     := src/cdspeedctl.c

PREFIX  ?= /usr

all: print_flags $(TARGET)

print_flags:
	@echo "[+] Build configuration:"
	@echo "    CC     = $(CC)"
	@echo "    CFLAGS = $(CFLAGS)"

$(TARGET): $(SRC)
	@echo "[+] Compiling $(TARGET)"
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@echo "[+] Cleaning build artifacts"
	rm -f $(TARGET)

install: $(TARGET)
	@echo "[+] Installing to $(DESTDIR)$(PREFIX)/bin/$(TARGET)"
	install -D -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: all clean install print_flags
