# Module 4: Sockets & Binary Protocols

```
┌─────────────────────────────────────────────────────────────────┐
│  "In JavaScript, fetch() hides the network stack.              │
│   In C, you speak directly to the kernel."                     │
└─────────────────────────────────────────────────────────────────┘
```

## Learning Goals

By the end of this module, you will understand:
1. The socket lifecycle (BLAB: Bind, Listen, Accept, Begin)
2. File descriptors and why sockets are "files" in Unix
3. Binary serialization (sending structs over the network)
4. Struct padding/packing for cross-platform compatibility
5. Basic network architecture for games

---

## Concept 1: What is a Socket?

### The JavaScript Mental Model

```javascript
// JavaScript hides the complexity
const ws = new WebSocket('ws://server:8080');
ws.send(JSON.stringify({ x: 100, y: 200 }));  // Magic!
ws.onmessage = (event) => { /* data arrives */ };
```

### The C Reality

A socket is a **file descriptor** - a number that represents an open communication channel.

```
┌─────────────────────────────────────────────────────────────────┐
│                    FILE DESCRIPTORS                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  In Unix, EVERYTHING is a file:                                 │
│                                                                 │
│  fd 0 ──▶ stdin  (keyboard input)                              │
│  fd 1 ──▶ stdout (terminal output)                             │
│  fd 2 ──▶ stderr (error output)                                │
│  fd 3 ──▶ Your log file (if opened)                            │
│  fd 4 ──▶ A SOCKET! (network connection)                       │
│                                                                 │
│  You can read/write to sockets just like files:                 │
│      write(socket_fd, "Hello", 5);                             │
│      read(socket_fd, buffer, sizeof(buffer));                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Concept 2: The BLAB Lifecycle (Server)

Remember the mnemonic: **B**ind, **L**isten, **A**ccept, **B**egin

```c
// 1. BIND - Associate socket with an address (IP + port)
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
bind(server_fd, (struct sockaddr*)&address, sizeof(address));

// 2. LISTEN - Start accepting connections
listen(server_fd, BACKLOG);

// 3. ACCEPT - Wait for a client to connect (BLOCKING!)
int client_fd = accept(server_fd, ...);  // Returns NEW socket for this client

// 4. BEGIN - Communicate with the client
while (running) {
    recv(client_fd, buffer, size, 0);  // Read data
    // Process data...
    send(client_fd, response, size, 0);  // Send response
}
```

### Visual Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    SERVER LIFECYCLE                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────┐     socket()     ┌──────────┐                    │
│  │  START   │ ───────────────▶ │  SOCKET  │                    │
│  └──────────┘                  └────┬─────┘                    │
│                                     │ bind()                   │
│                                     ▼                          │
│                               ┌──────────┐                     │
│                               │  BOUND   │                     │
│                               └────┬─────┘                     │
│                                    │ listen()                  │
│                                    ▼                           │
│                              ┌───────────┐                     │
│                              │ LISTENING │◀─────────┐          │
│                              └────┬──────┘          │          │
│                                   │ accept()        │          │
│                                   ▼                 │          │
│                              ┌───────────┐          │          │
│                              │ CONNECTED │          │          │
│                              │  (client) │──────────┘          │
│                              └───────────┘   close()           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Concept 3: Client Connection

The client's lifecycle is simpler:

```c
// 1. Create socket
int sock = socket(AF_INET, SOCK_STREAM, 0);

// 2. Connect to server
connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

// 3. Communicate
send(sock, data, size, 0);
recv(sock, buffer, size, 0);

// 4. Close
close(sock);
```

---

## Concept 4: Binary Protocols (Not JSON!)

### Why Not JSON?

```javascript
// JavaScript: Easy, but wasteful
const player = { x: 100.5, y: 200.25, health: 100 };
ws.send(JSON.stringify(player));
// Sends: '{"x":100.5,"y":200.25,"health":100}'
// That's 37 bytes of ASCII text!
```

```c
// C: Send raw binary data
typedef struct {
    float x;      // 4 bytes
    float y;      // 4 bytes
    int health;   // 4 bytes
} PlayerState;    // Total: 12 bytes (or less with packing)

PlayerState player = { 100.5f, 200.25f, 100 };
send(sock, &player, sizeof(PlayerState), 0);
// Sends exactly 12 bytes of raw data
```

### Benefits of Binary

| Aspect | JSON | Binary |
|--------|------|--------|
| Size | 37+ bytes | 12 bytes |
| Parse time | Requires parser | Zero (it's already the struct!) |
| Precision | String rounding issues | Exact float bits |
| Network cost | 3x more data | Minimal |

### The Danger: Struct Padding

```c
typedef struct {
    char type;     // 1 byte
    int value;     // 4 bytes
} Message;

// You might expect 5 bytes, but...
// sizeof(Message) == 8 bytes!
//
// Memory layout:
// ┌──────┬───────────┬──────────────┐
// │ type │ PADDING   │    value     │
// │ 1 B  │ 3 bytes   │   4 bytes    │
// └──────┴───────────┴──────────────┘
//
// The compiler adds padding for alignment!
```

### Solution: Packing

```c
// Use __attribute__((packed)) to remove padding
typedef struct __attribute__((packed)) {
    char type;     // 1 byte
    int value;     // 4 bytes
} Message;         // Now exactly 5 bytes!

// WARNING: Packed structs may be slower to access on some architectures
// Use them only for network data, not for general game state
```

---

## Concept 5: Endianness

### The Problem

```
Different CPUs store multi-byte values differently:

Little-Endian (x86, ARM):  0x12345678 stored as [78] [56] [34] [12]
Big-Endian (Network):      0x12345678 stored as [12] [34] [56] [78]

If you send raw integers between different architectures,
the bytes get misinterpreted!
```

### The Solution: Network Byte Order

```c
#include <arpa/inet.h>

// htons = Host TO Network Short (16-bit)
// htonl = Host TO Network Long (32-bit)
// ntohs = Network TO Host Short
// ntohl = Network TO Host Long

uint16_t port = 8080;
uint16_t network_port = htons(port);  // Convert before sending

// For floats, there's no standard function
// Many games just document "use little-endian"
// Or use a serialization library
```

---

## Concept 6: The Message Protocol

For our game, we'll use a simple message format:

```c
typedef enum {
    MSG_NONE = 0,        // Invalid/empty message
    MSG_CONNECT,         // Client -> Server: Request to join
    MSG_CONNECT_ACK,     // Server -> Client: Accept/reject connection
    MSG_DISCONNECT,      // Either direction: Player leaving
    MSG_PLAYER_INPUT,    // Client -> Server: Player pressed keys
    MSG_GAME_STATE,      // Server -> Client: Current world state
    MSG_PING,            // Client -> Server: Latency check
    MSG_PONG             // Server -> Client: Latency response
} MessageType;

// All messages start with this header
typedef struct __attribute__((packed)) {
    uint8_t type;        // MessageType
    uint16_t length;     // Length of payload after header
} MessageHeader;

// Client sends this after TCP connect
typedef struct __attribute__((packed)) {
    uint8_t version;     // Protocol version (for compatibility)
    char name[16];       // Player name
} ConnectMsg;

// Server responds with acceptance or rejection
typedef struct __attribute__((packed)) {
    uint8_t success;     // 1 = accepted, 0 = rejected
    uint8_t player_id;   // Assigned player ID
    uint8_t reason;      // If rejected: 0 = full, 1 = version mismatch
} ConnectAckMsg;

// Client sends input state every frame
typedef struct __attribute__((packed)) {
    uint8_t player_id;   // Which player
    uint8_t input_flags; // Bitfield: UP|DOWN|LEFT|RIGHT|FIRE
    uint8_t weapon_type; // Current weapon (for fire rate/pattern)
    uint32_t sequence;   // Message ordering
} PlayerInputMsg;

// Server sends complete world state
typedef struct __attribute__((packed)) {
    uint32_t tick;           // Server tick number
    uint32_t your_sequence;  // Last input processed
    uint8_t player_count;    // Number of players
    uint8_t bullet_count;    // Number of bullets
    // Followed by: PlayerState[], then BulletState[]
} GameStateMsg;
```

---

## Concept 7: Connection Handshake

The connection follows a strict handshake protocol:

```
┌─────────────────────────────────────────────────────────────────┐
│                    CONNECTION HANDSHAKE                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   CLIENT                              SERVER                     │
│     │                                    │                       │
│     │──── TCP connect() ────────────────▶│                       │
│     │                                    │                       │
│     │──── MSG_CONNECT ──────────────────▶│                       │
│     │     (version, name)                │  Validate version     │
│     │                                    │  Find player slot     │
│     │◀─── MSG_CONNECT_ACK ──────────────│                       │
│     │     (success, player_id)           │                       │
│     │                                    │                       │
│     │════ CONNECTED ════════════════════│                       │
│     │                                    │                       │
│     │──── MSG_PLAYER_INPUT ────────────▶│                       │
│     │◀─── MSG_GAME_STATE ───────────────│                       │
│     │         (repeat)                   │                       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

**Critical**: The server MUST read MSG_CONNECT before sending MSG_CONNECT_ACK.
Skipping this causes message framing issues (bytes pile up, messages misalign).

---

## Concept 8: Weapon Synchronization

Different weapons have different behaviors that must be consistent:

```c
// Weapon-specific configurations (server and client must match!)
#define SPREAD_FIRE_RATE    3.0f    // 3 shots/sec, 3 bullets in fan
#define RAPID_FIRE_RATE     10.0f   // 10 shots/sec, 1 bullet
#define LASER_FIRE_RATE     1.5f    // 1.5 shots/sec, 1 large bullet

// Fire cooldown = 1.0 / fire_rate
// Spread: 0.333s between shots
// Rapid:  0.1s between shots
// Laser:  0.667s between shots
```

The client sends `weapon_type` with every input message so the server knows:
1. Which fire rate to use (cooldown between shots)
2. Which bullet pattern to spawn (spread = 3 bullets, others = 1)
3. Which bullet speed to use (400, 600, or 800)

---

## Concept 9: Bullet Synchronization

Bullets are synchronized from server to all clients:

```c
typedef struct __attribute__((packed)) {
    uint8_t owner_id;    // Which player fired this
    float x, y;          // Position
    float vx, vy;        // Velocity
    uint8_t weapon_type; // For rendering (color, size)
} BulletState;
```

The server:
1. Creates bullets when INPUT_FIRE is set (respecting cooldowns)
2. Updates bullet positions each tick
3. Removes bullets that expire or leave the screen
4. Sends all active bullets in MSG_GAME_STATE

Clients render bullets based on `weapon_type`:
- Spread: Yellow-orange, medium size
- Rapid: Cyan, thin
- Laser: Magenta/pink, large

---

## Concept 10: Handling Disconnections Gracefully

### The SIGPIPE Problem

When sending to a disconnected client, the OS sends SIGPIPE which kills your process!

```c
// Solution: Ignore SIGPIPE in main()
signal(SIGPIPE, SIG_IGN);

// Now send() returns -1 instead of killing the process
if (send(client_socket, data, size, 0) < 0) {
    // Handle error gracefully - disconnect this client
    close(client_socket);
    player->active = 0;
}
```

### Non-Blocking Sockets and EAGAIN

With non-blocking sockets, `recv()` returns -1 with `errno = EAGAIN` when no data is available:

```c
int bytes = recv(socket, buffer, size, 0);

if (bytes == 0) {
    // Connection closed by peer - disconnect
} else if (bytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data available right now - that's OK, try again later
        return;
    }
    // Actual error - disconnect
}
// bytes > 0: Process the data
```

**Critical**: Don't treat EAGAIN as a disconnect! It just means "no data yet".

---

## The Deliverable

Two separate programs:

### Server
- Listens on port 8080
- Accepts client connections
- Receives player input
- Sends back game state

### Client
- Connects to server
- Sends input (keyboard state)
- Receives and displays world state

```
┌─────────────────────────────────────────────────────────────────┐
│  Terminal 1 (Server):                                           │
│  $ ./server                                                     │
│  Listening on port 8080...                                      │
│  Client connected from 127.0.0.1:54321                          │
│  Received input: UP | FIRE                                      │
│  Sending state: pos=(400.0, 250.0)                             │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│  Terminal 2 (Client):                                           │
│  $ ./client 127.0.0.1                                           │
│  Connected to server!                                           │
│  Sending input...                                               │
│  Received state: pos=(400.0, 250.0), health=100                │
└─────────────────────────────────────────────────────────────────┘
```

---

## File Structure

```
module4_networking/
├── server.c         # Server implementation
├── client.c         # Client implementation
├── protocol.h       # Shared message definitions
├── network.h        # Socket helper functions
├── network.c        # Socket implementation
└── Makefile         # Builds both server and client
```

Now let's look at the code!
