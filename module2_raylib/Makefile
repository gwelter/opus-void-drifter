# Makefile for Void Drifter Module 2: Raylib Game Loop
#
# CONCEPT: Linking External Libraries
# ===================================
# In JavaScript, you'd: npm install raylib
# In C, you need to tell the compiler:
#     1. WHERE to find header files (-I flag)
#     2. WHERE to find library files (-L flag)
#     3. WHICH libraries to link (-l flag)
#
# Raylib is typically installed via:
#     macOS:  brew install raylib
#     Linux:  sudo apt install libraylib-dev
#     Windows: Download from raylib.com

# Compiler
CC = cc

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g -O0

# Raylib configuration
# ====================
# pkg-config is a helper that returns the correct flags for installed libraries
# If raylib was installed via brew/apt, this should work:
#
# $(shell ...) executes a shell command and captures output
# pkg-config --cflags raylib  → returns include paths
# pkg-config --libs raylib    → returns library flags

# Check if pkg-config knows about raylib
RAYLIB_EXISTS := $(shell pkg-config --exists raylib && echo yes || echo no)

ifeq ($(RAYLIB_EXISTS),yes)
    # Use pkg-config for flags
    RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib)
    RAYLIB_LIBS := $(shell pkg-config --libs raylib)
else
    # Fallback: Try common installation paths
    # macOS Homebrew typically installs to /opt/homebrew (Apple Silicon) or /usr/local (Intel)
    UNAME_S := $(shell uname -s)
    UNAME_M := $(shell uname -m)

    ifeq ($(UNAME_S),Darwin)
        # macOS
        ifeq ($(UNAME_M),arm64)
            # Apple Silicon
            RAYLIB_CFLAGS := -I/opt/homebrew/include
            RAYLIB_LIBS := -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
        else
            # Intel Mac
            RAYLIB_CFLAGS := -I/usr/local/include
            RAYLIB_LIBS := -L/usr/local/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
        endif
    else ifeq ($(UNAME_S),Linux)
        # Linux
        RAYLIB_CFLAGS := -I/usr/local/include
        RAYLIB_LIBS := -L/usr/local/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    else
        # Windows (MinGW)
        RAYLIB_CFLAGS :=
        RAYLIB_LIBS := -lraylib -lopengl32 -lgdi32 -lwinmm
    endif
endif

# Add math library (needed for sinf, sqrtf, etc.)
LIBS = $(RAYLIB_LIBS) -lm

# Target executable
TARGET = void_drifter_m2

# Source files
SOURCES = main.c player.c textures.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Header files
HEADERS = player.h textures.h game_state.h

# Default target
all: check-raylib $(TARGET)

# Check if raylib is available
.PHONY: check-raylib
check-raylib:
	@echo "Checking Raylib installation..."
	@if [ "$(RAYLIB_EXISTS)" = "yes" ]; then \
		echo "  Raylib found via pkg-config"; \
	else \
		echo "  Raylib not found via pkg-config, using fallback paths"; \
	fi
	@echo "  CFLAGS: $(RAYLIB_CFLAGS)"
	@echo "  LIBS: $(RAYLIB_LIBS)"
	@echo ""

# Link object files into executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo ""
	@echo "Build successful! Run with: ./$(TARGET)"
	@echo ""

# Compile .c files to .o files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -c $< -o $@

# Clean build artifacts
.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned build artifacts"

# Run the program
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# Debug info
.PHONY: info
info:
	@echo "=== Build Configuration ==="
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Raylib CFLAGS: $(RAYLIB_CFLAGS)"
	@echo "Raylib LIBS: $(RAYLIB_LIBS)"
	@echo "Sources: $(SOURCES)"
	@echo "Target: $(TARGET)"
	@echo ""
	@echo "=== Commands ==="
	@echo "  make        - Build the program"
	@echo "  make run    - Build and run"
	@echo "  make clean  - Remove build artifacts"
	@echo "  make info   - Show this information"
	@echo ""
	@echo "=== Raylib Installation ==="
	@echo "If build fails, ensure Raylib is installed:"
	@echo "  macOS:   brew install raylib"
	@echo "  Ubuntu:  sudo apt install libraylib-dev"
	@echo "  Windows: Download from https://www.raylib.com/"

.PHONY: help
help: info
