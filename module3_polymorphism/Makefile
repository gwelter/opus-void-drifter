# Makefile for Void Drifter Module 3: Function Pointers & Strategy Pattern

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

LIBS = $(RAYLIB_LIBS) -lm

# Target
TARGET = void_drifter_m3

# Sources - Note the additional weapon.c and bullet.c
SOURCES = main.c player.c weapon.c bullet.c textures.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = player.h weapon.h bullet.h textures.h

# Default target
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo ""
	@echo "Build successful!"
	@echo "Run with: ./$(TARGET)"
	@echo ""
	@echo "This module demonstrates Function Pointers (Strategy Pattern)"
	@echo "  - Press 1/2/3 to switch weapons"
	@echo "  - Each weapon has a different fire function pointer"
	@echo ""

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned"

.PHONY: run
run: $(TARGET)
	./$(TARGET)

.PHONY: info
info:
	@echo "=== Module 3: Function Pointers ==="
	@echo ""
	@echo "Key Concept:"
	@echo "  The Weapon struct contains a function pointer 'fire'."
	@echo "  Different weapons point to different fire functions:"
	@echo "    - WEAPON_SPREAD -> spread_fire()"
	@echo "    - WEAPON_RAPID  -> rapid_fire()"
	@echo "    - WEAPON_LASER  -> laser_fire()"
	@echo ""
	@echo "  When player fires, we call: weapon->fire(position, bullets)"
	@echo "  The correct function is called based on equipped weapon!"
	@echo ""
	@echo "Files:"
	@echo "  weapon.h - FireFunc typedef, Weapon struct"
	@echo "  weapon.c - Fire function implementations"
	@echo "  player.c - Uses weapon->fire() without knowing which weapon"
	@echo ""

.PHONY: help
help: info
