# Makefile for Void Drifter Module 1: Memory Management
#
# CONCEPT: Makefiles
# ==================
# In JavaScript, you might use npm scripts or webpack.
# In C, we use Make - a build automation tool from 1976 that's still standard.
#
# Make reads this file and:
#   1. Determines what needs to be built
#   2. Runs the necessary compiler commands
#   3. Only rebuilds files that changed (incremental builds)
#
# Basic syntax:
#   target: dependencies
#       command (MUST be indented with a TAB, not spaces!)
#
# Run with: make          (builds default target)
#           make clean    (removes build artifacts)
#           make run      (builds and runs)

# Compiler configuration
# ======================
# CC: The C compiler to use
#     - 'cc' is a symlink to your system's default C compiler
#     - On macOS, this is typically clang
#     - On Linux, this is typically gcc
CC = cc

# CFLAGS: Compiler flags
# ======================
# -Wall      Enable all common warnings (highly recommended!)
# -Wextra    Enable extra warnings beyond -Wall
# -pedantic  Enforce strict ISO C compliance
# -std=c11   Use the C11 standard (we specified this in the course)
# -g         Include debug symbols (for debuggers like lldb/gdb)
# -O0        No optimization (easier to debug)
#
# For release builds, you'd change -O0 to -O2 or -O3 for optimization
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g -O0

# Target executable name
TARGET = void_drifter_m1

# Source files
# ============
# List all .c files that need to be compiled
# The order doesn't matter for compilation, just for reading
SOURCES = main.c bullet.c list.c

# Object files
# ============
# Each .c file compiles to a .o (object) file
# $(SOURCES:.c=.o) is Make syntax for "replace .c with .o in SOURCES"
#
# Compilation flow:
#   main.c   -> main.o   -\
#   bullet.c -> bullet.o  |-> void_drifter_m1 (linked executable)
#   list.c   -> list.o   -/
OBJECTS = $(SOURCES:.c=.o)

# Header files (for dependency tracking)
HEADERS = bullet.h list.h

# Default target
# ==============
# The first target is the default when you just run 'make'
# 'all' is a conventional name for "build everything"
all: $(TARGET)

# Link object files into executable
# =================================
# $@ = the target name (void_drifter_m1)
# $^ = all dependencies (main.o bullet.o list.o)
#
# This rule says: "To build void_drifter_m1, link all .o files"
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo ""
	@echo "Build successful! Run with: ./$(TARGET)"
	@echo ""

# Compile .c files to .o files
# ============================
# This is a "pattern rule" - it applies to any .c -> .o conversion
# $< = the first dependency (the .c file)
# $@ = the target (the .o file)
#
# The $(HEADERS) dependency means: if ANY header changes, recompile.
# This is a simple approach; for large projects, you'd use auto-dependencies.
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
# =====================
# Remove all generated files
# .PHONY means this isn't a real file, it's just a command name
.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned build artifacts"

# Run the program
# ===============
# Builds if necessary, then runs
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# Run with Valgrind (Linux) or leaks (macOS)
# ==========================================
.PHONY: memcheck
memcheck: $(TARGET)
	@echo "Running memory check..."
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET); \
	elif command -v leaks >/dev/null 2>&1; then \
		leaks --atExit -- ./$(TARGET); \
	else \
		echo "Neither valgrind nor leaks found. Install one for memory checking."; \
	fi

# Debug build info
# ================
.PHONY: info
info:
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "Target: $(TARGET)"
	@echo ""
	@echo "Build commands:"
	@echo "  make        - Build the program"
	@echo "  make run    - Build and run"
	@echo "  make clean  - Remove build artifacts"
	@echo "  make memcheck - Run with memory leak detection"

# Help target
# ===========
.PHONY: help
help: info
