# Makefile for Void Drifter Module 5: Concurrency
#
# This module builds the complete game with:
#     - Raylib graphics
#     - Network client
#     - Threading via pthreads
#
# To run:
#     OFFLINE: ./void_drifter
#     ONLINE:  ./void_drifter --online --host 127.0.0.1 --port 8080
#
# For multiplayer, also run the server from Module 4:
#     Terminal 1: cd ../module4_networking && ./server
#     Terminal 2: ./void_drifter --online

CC = cc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g -O0

# Raylib configuration
RAYLIB_EXISTS := $(shell pkg-config --exists raylib && echo yes || echo no)

ifeq ($(RAYLIB_EXISTS),yes)
    RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib)
    RAYLIB_LIBS := $(shell pkg-config --libs raylib)
else
    UNAME_S := $(shell uname -s)
    UNAME_M := $(shell uname -m)

    ifeq ($(UNAME_S),Darwin)
        ifeq ($(UNAME_M),arm64)
            RAYLIB_CFLAGS := -I/opt/homebrew/include
            RAYLIB_LIBS := -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
        else
            RAYLIB_CFLAGS := -I/usr/local/include
            RAYLIB_LIBS := -L/usr/local/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
        endif
    else ifeq ($(UNAME_S),Linux)
        RAYLIB_CFLAGS := -I/usr/local/include
        RAYLIB_LIBS := -L/usr/local/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    else
        RAYLIB_CFLAGS :=
        RAYLIB_LIBS := -lraylib -lopengl32 -lgdi32 -lwinmm
    endif
endif

# Thread library
THREAD_LIBS = -lpthread

# Math library
MATH_LIBS = -lm

# All libraries
LIBS = $(RAYLIB_LIBS) $(THREAD_LIBS) $(MATH_LIBS)

# Target
TARGET = void_drifter

# Source files
SOURCES = main.c \
          shared_state.c \
          network_client.c \
          network.c \
          weapon.c \
          bullet.c \
          textures.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Header files
HEADERS = shared_state.h \
          network_client.h \
          network.h \
          protocol.h \
          weapon.h \
          bullet.h \
          textures.h

# Default target
all: $(TARGET)
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║              BUILD SUCCESSFUL!                             ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "To play OFFLINE (single player):"
	@echo "  ./$(TARGET)"
	@echo ""
	@echo "To play ONLINE (multiplayer):"
	@echo "  1. Start the server: cd ../module4_networking && ./server"
	@echo "  2. Start the game:   ./$(TARGET) --online"
	@echo ""
	@echo "Options:"
	@echo "  --online, -o     Connect to server"
	@echo "  --host HOST      Server address (default: 127.0.0.1)"
	@echo "  --port PORT      Server port (default: 8080)"
	@echo ""

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned"

.PHONY: run
run: $(TARGET)
	./$(TARGET)

.PHONY: run-online
run-online: $(TARGET)
	./$(TARGET) --online

# Info about the module
.PHONY: info
info:
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║      MODULE 5: CONCURRENCY - THREADS & MUTEXES             ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "KEY CONCEPTS:"
	@echo ""
	@echo "  1. BLOCKING PROBLEM"
	@echo "     - recv() blocks until data arrives"
	@echo "     - This would freeze the game loop!"
	@echo ""
	@echo "  2. THREADING SOLUTION"
	@echo "     - Network code runs in separate thread"
	@echo "     - Game loop continues at 60 FPS"
	@echo ""
	@echo "  3. MUTEX SYNCHRONIZATION"
	@echo "     - SharedState is protected by pthread_mutex"
	@echo "     - Prevents race conditions"
	@echo "     - Lock before access, unlock after"
	@echo ""
	@echo "FILES:"
	@echo "  shared_state.h/c    - Mutex-protected shared data"
	@echo "  network_client.h/c  - Network thread implementation"
	@echo "  main.c              - Game loop + thread management"
	@echo ""
	@echo "THREADING PATTERN:"
	@echo ""
	@echo "  MAIN THREAD          |  NETWORK THREAD"
	@echo "  ───────────          |  ──────────────"
	@echo "  • Input              |  • recv() (can block)"
	@echo "  • Update             |  • send()"
	@echo "  • Render             |  • Update SharedState"
	@echo "         \\             |             /"
	@echo "          \\            |            /"
	@echo "           └──── MUTEX ────────────┘"
	@echo ""

.PHONY: help
help: info

# Summary of the entire course
.PHONY: course-summary
course-summary:
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║         VOID DRIFTER: COMPLETE COURSE SUMMARY              ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "MODULE 1: MEMORY"
	@echo "  • malloc/free"
	@echo "  • Linked lists"
	@echo "  • Memory leaks"
	@echo ""
	@echo "MODULE 2: RAYLIB"
	@echo "  • Game loop"
	@echo "  • Procedural textures"
	@echo "  • Delta time"
	@echo ""
	@echo "MODULE 3: POLYMORPHISM"
	@echo "  • Function pointers"
	@echo "  • Strategy pattern"
	@echo "  • Weapon system"
	@echo ""
	@echo "MODULE 4: NETWORKING"
	@echo "  • POSIX sockets"
	@echo "  • Binary protocol"
	@echo "  • Client/server"
	@echo ""
	@echo "MODULE 5: CONCURRENCY"
	@echo "  • pthreads"
	@echo "  • Mutexes"
	@echo "  • Thread-safe state"
	@echo ""
	@echo "You've built a networked multiplayer game from scratch!"
	@echo ""
