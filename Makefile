CC ?= gcc

SRC_DIR := src
INCLUDE_DIR := include
OUTPUT_DIR := output

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

# Prefer compiler target macros so cross-compilation selects the target
# backend instead of the build host. If probing fails, fall back to uname; if
# probing succeeds without a known OS macro, use the unknown backend.
CC_TARGET_MACROS := $(shell $(CC) -dM -E -x c /dev/null 2>/dev/null)
ifneq ($(findstring __APPLE__,$(CC_TARGET_MACROS)),)
    CC_TARGET_OS := Darwin
else ifneq ($(findstring _WIN32,$(CC_TARGET_MACROS)),)
    CC_TARGET_OS := Windows
else ifneq ($(findstring __CYGWIN__,$(CC_TARGET_MACROS)),)
    CC_TARGET_OS := CYGWIN
else ifneq ($(findstring _AIX,$(CC_TARGET_MACROS)),)
    CC_TARGET_OS := AIX
else ifneq ($(findstring __sun,$(CC_TARGET_MACROS)),)
    CC_TARGET_OS := SunOS
else ifneq ($(findstring __linux__,$(CC_TARGET_MACROS)),)
    CC_TARGET_OS := Linux
else ifneq ($(strip $(CC_TARGET_MACROS)),)
    CC_TARGET_OS := Unknown
endif
ifneq ($(findstring mingw,$(CC)),)
    CC_NAME_OS := Windows
else ifneq ($(findstring MINGW,$(CC)),)
    CC_NAME_OS := Windows
endif
UNAME_S := $(shell uname -s 2>/dev/null)
SYS_OS ?= $(or $(filter-out Unknown,$(CC_TARGET_OS)),$(CC_NAME_OS),$(CC_TARGET_OS),$(UNAME_S),Unknown)

BACKEND_SRC := $(wildcard $(SRC_DIR)/eatmemory.*.c)
COMMON_SRC := $(filter-out $(BACKEND_SRC),$(wildcard $(SRC_DIR)/*.c))

ifndef SYS_SRC
    ifeq ($(SYS_OS),SunOS)
        SYS_SRC := $(SRC_DIR)/eatmemory.solaris.c
        SYS_BACKEND := SunOS
    else ifeq ($(SYS_OS),AIX)
        SYS_SRC := $(SRC_DIR)/eatmemory.aix.c
        SYS_BACKEND := AIX
    else ifeq ($(SYS_OS),Linux)
        SYS_SRC := $(SRC_DIR)/eatmemory.linux.c
        SYS_BACKEND := Linux
    else ifeq ($(SYS_OS),Darwin)
        SYS_SRC := $(SRC_DIR)/eatmemory.apple.c
        SYS_BACKEND := Darwin
    else ifneq ($(filter Windows MINGW% MSYS% CYGWIN%,$(SYS_OS)),)
        SYS_SRC := $(SRC_DIR)/eatmemory.windows.c
        SYS_BACKEND := Windows
    else
        SYS_SRC := $(SRC_DIR)/eatmemory.unknown.c
        SYS_BACKEND := Unkown OS
    endif
endif
SYS_BACKEND ?= $(patsubst eatmemory.%.c,%,$(notdir $(SYS_SRC)))
ifeq ($(SYS_BACKEND),unknown)
    SYS_BACKEND := Unkown OS
endif
CFLAGS += -DSYSMEM_BACKEND='"$(SYS_BACKEND)"'

ifeq ($(notdir $(SYS_SRC)),eatmemory.aix.c)
    LDFLAGS += -lperfstat
endif

ifneq ($(filter eatmemory.windows.c,$(notdir $(SYS_SRC))),)
    EXE_SUFFIX := .exe
else ifeq ($(CC_NAME_OS),Windows)
    EXE_SUFFIX := .exe
else
    EXE_SUFFIX :=
endif

SRC := $(COMMON_SRC) $(SYS_SRC)
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
	@echo "Variables:"
	@echo "  SYS_OS        - Override backend auto-detection (Linux, Darwin, AIX, SunOS, Windows, CYGWIN*, Unknown)"
	@echo "  SYS_SRC       - Override the backend source file directly"

.PHONY: all install clean help
