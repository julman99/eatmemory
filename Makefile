CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -O3
LDFLAGS :=
SRC_DIR := src
INCLUDE_DIR := include
OUTPUT_DIR := output
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OUTPUT_DIR)/%.o,$(SRC))
EXE := $(OUTPUT_DIR)/eatmemory
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





