# Makefile for Void Drifter Module 4: Networking
#
# This module builds TWO separate executables:
#     - server: The game server
#     - client: The game client
#
# To test, run in separate terminals:
#     Terminal 1: ./server
#     Terminal 2: ./client
#     Terminal 3: ./client  (second player)

CC = cc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g -O0

# Network libraries vary by platform
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    # Linux might need explicit socket library linking
    NETLIBS =
else ifeq ($(UNAME_S),Darwin)
    # macOS doesn't need extra libs
    NETLIBS =
else
    # Windows (MinGW) would need -lws2_32
    NETLIBS = -lws2_32
endif

# Targets
SERVER = server
CLIENT = client

# Source files
COMMON_SOURCES = network.c
SERVER_SOURCES = server.c $(COMMON_SOURCES)
CLIENT_SOURCES = client.c $(COMMON_SOURCES)

# Object files
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)

# Header files
HEADERS = protocol.h network.h

# Default: build both
all: $(SERVER) $(CLIENT)
	@echo ""
	@echo "Build successful!"
	@echo ""
	@echo "To test networking:"
	@echo "  Terminal 1:  ./server"
	@echo "  Terminal 2:  ./client"
	@echo "  Terminal 3:  ./client  (optional, second player)"
	@echo ""
	@echo "The server runs on port 8080 by default."
	@echo "Use './server PORT' or './client HOST PORT' to customize."
	@echo ""

# Build server
$(SERVER): server.o network.o
	$(CC) $(CFLAGS) -o $@ $^ $(NETLIBS)
	@echo "Built server executable"

# Build client
$(CLIENT): client.o network.o
	$(CC) $(CFLAGS) -o $@ $^ $(NETLIBS)
	@echo "Built client executable"

# Compile rules
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
.PHONY: clean
clean:
	rm -f *.o $(SERVER) $(CLIENT)
	@echo "Cleaned"

# Run server
.PHONY: run-server
run-server: $(SERVER)
	./$(SERVER)

# Run client (connects to localhost)
.PHONY: run-client
run-client: $(CLIENT)
	./$(CLIENT)

# Run both (server in background, then client)
.PHONY: demo
demo: all
	@echo "Starting server in background..."
	@./$(SERVER) &
	@sleep 1
	@echo "Starting client..."
	@./$(CLIENT)
	@echo "Demo complete. Server may still be running (use 'pkill server' to stop)"

# Info
.PHONY: info
info:
	@echo "=== Module 4: Sockets & Binary Protocols ==="
	@echo ""
	@echo "Key Concepts:"
	@echo "  1. BLAB Server Lifecycle: Bind, Listen, Accept, Begin"
	@echo "  2. Binary Protocol: Sending raw structs instead of JSON"
	@echo "  3. Struct Packing: __attribute__((packed)) for network data"
	@echo "  4. Endianness: Network byte order (htons, ntohs)"
	@echo ""
	@echo "Files:"
	@echo "  protocol.h - Message definitions (shared)"
	@echo "  network.h/c - Socket wrapper functions"
	@echo "  server.c - Game server (authoritative)"
	@echo "  client.c - Game client (sends input, receives state)"
	@echo ""
	@echo "Testing:"
	@echo "  1. Open two terminal windows"
	@echo "  2. In first:  ./server"
	@echo "  3. In second: ./client"
	@echo "  4. Use WASD in client to move"
	@echo ""

.PHONY: help
help: info
