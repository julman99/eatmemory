CC ?= gcc

# Version string is derived from the most recent v* tag plus the number of
# commits since, the abbreviated commit hash, and a -dirty suffix if the
# working tree has uncommitted changes. The leading 'v' is stripped so the
# binary displays "eatmemory 0.2.0" rather than "eatmemory v0.2.0", matching
# the historical display and the homebrew/semver convention. Falls back to
# "unknown" when built outside a git checkout (e.g., from a source tarball).
# Override with `make VERSION=1.2.3` if you need to bake in a specific value.
VERSION ?= $(shell (git describe --tags --match 'v*' --always --dirty --abbrev=8 2>/dev/null || echo unknown) | sed 's/^v//')

# EXTRA_CFLAGS lets callers append flags without overriding the defaults.
# Example: `make EXTRA_CFLAGS=-m32`. The flag also reaches the link step
# because $(CFLAGS) appears on the link command line below.
EXTRA_CFLAGS ?=

CFLAGS := -Wall -Wextra -std=c99 -O2 -g $(EXTRA_CFLAGS) -DVERSION='"$(VERSION)"'
LDFLAGS :=

ifneq ($(findstring mingw,$(CC)),)
    EXE_SUFFIX := .exe
else
    EXE_SUFFIX :=
endif

SRC_DIR := src
INCLUDE_DIR := include
OUTPUT_DIR := output
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OUTPUT_DIR)/%.o,$(SRC))
EXE := $(OUTPUT_DIR)/eatmemory$(EXE_SUFFIX)

PREFIX := /usr/local
INSTALL_DIR := $(PREFIX)/bin

# Default target: Build the eatmemory program
all: $(EXE)

$(OUTPUT_DIR)/%.o: $(SRC_DIR)/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE_DIR)

$(EXE): $(OBJ) | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Install the executable to the specified PREFIX directory
install: $(EXE)
	mkdir -p $(INSTALL_DIR)
	install -m 755 $< $(INSTALL_DIR)

# Clean generated files
clean:
	rm -rf $(OUTPUT_DIR)

# Display help message
help:
	@echo "Usage: make [target] [PREFIX=/your/installation/path]"
	@echo "Targets:"
	@echo "  all (default) - Build the eatmemory program"
	@echo "  install       - Install the executable to PREFIX/bin"
	@echo "  clean         - Remove generated files"
	@echo "  help          - Display this help message"

.PHONY: all install clean help





