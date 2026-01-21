# Module 5: Concurrency - Threads & Mutexes

```
┌─────────────────────────────────────────────────────────────────┐
│  "In JavaScript, the event loop handles concurrency magically. │
│   In C, you manage threads manually - with all the danger."    │
└─────────────────────────────────────────────────────────────────┘
```

## Learning Goals

By the end of this module, you will understand:
1. Why blocking calls break game loops
2. How threads allow parallel execution
3. The critical danger of race conditions
4. How mutexes act as "traffic signals" for shared data
5. Thread-safe patterns for game networking

---

## Concept 1: The Blocking Problem

### The Issue

```c
// This BREAKS the game!
while (!WindowShouldClose()) {
    // This blocks until data arrives - could be FOREVER!
    recv(socket, buffer, size, 0);  // GAME FREEZES HERE

    // Draw never gets called while waiting for network
    BeginDrawing();
    // ...
    EndDrawing();
}
```

### Why It Matters

```
Normal Game Loop (60 FPS):

Frame 1 ──▶ Frame 2 ──▶ Frame 3 ──▶ Frame 4
  16ms       16ms        16ms        16ms

Blocked Game Loop:

Frame 1 ──▶ recv() ──────────────────────────────▶ Frame 2
  16ms              BLOCKED (500ms+)                16ms

Result: Game appears frozen, player thinks it crashed!
```

### The Solution: Threads

Run network code in a SEPARATE thread:

```
┌─────────────────────────────────────────────────────────────────┐
│                    THREADED ARCHITECTURE                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   MAIN THREAD (Render Loop)    │  NETWORK THREAD               │
│   ════════════════════════     │  ══════════════════           │
│   • Runs at 60 FPS             │  • Runs independently         │
│   • Handles input              │  • Blocks on recv() - OK!     │
│   • Updates game logic         │  • Sends data to server       │
│   • Renders graphics           │  • Receives server state      │
│                                │                                │
│         SHARED STATE ◀─────────┼──────────▶ SHARED STATE       │
│   (protected by mutex)         │        (protected by mutex)   │
│                                │                                │
└─────────────────────────────────────────────────────────────────┘
```

---

## Concept 2: Threads in C (pthreads)

### JavaScript's Mental Model: Web Workers

```javascript
// JavaScript: Web Workers run in separate threads
const worker = new Worker('network.js');
worker.postMessage({ type: 'connect', host: '...' });
worker.onmessage = (event) => { /* handle data */ };
```

### C's Approach: pthreads

```c
#include <pthread.h>

// The function that runs in the new thread
void* network_thread_func(void* arg) {
    while (running) {
        recv(socket, buffer, size, 0);  // Can block here!
        // Process received data...
    }
    return NULL;
}

int main(void) {
    pthread_t network_thread;

    // Create the thread
    pthread_create(&network_thread, NULL, network_thread_func, NULL);

    // Main thread continues with game loop
    while (!WindowShouldClose()) {
        // Game loop runs normally, unblocked
    }

    // Wait for network thread to finish
    pthread_join(network_thread, NULL);
}
```

### Thread Lifecycle

```
pthread_create() ──▶ Thread starts running
        │
        │ (thread executes its function)
        │
pthread_join()  ◀── Thread finishes
        │
        ▼
   (thread destroyed)
```

---

## Concept 3: The Race Condition

### The Danger: Two Threads, One Variable

```c
// DANGER! Both threads access 'player_x' without protection

// Network thread:
void* network_func(void* arg) {
    while (running) {
        player_x = received_data.x;  // WRITE
    }
}

// Main thread:
while (!WindowShouldClose()) {
    DrawPlayer(player_x, player_y);  // READ (might get partial write!)
}
```

### What Goes Wrong: Data Tearing

```
Imagine player_x is a 64-bit float (8 bytes).

Network thread writes bytes:     [1][2][3][4][5][6][7][8]
                                     ↑
                                     Main thread reads HERE
                                     Gets: [1][2][3][4][X][X][X][X]
                                           (first 4 bytes new, last 4 old!)

Result: player_x = garbage value = glitchy graphics!
```

### Analogy: The Intersection Problem

```
┌─────────────────────────────────────────────────────────────────┐
│  Without synchronization:                                       │
│                                                                 │
│     Thread A ───────────▶     │     ◀─────────── Thread B      │
│                           ╳   │   ╳                             │
│                  CRASH!   ╳   │   ╳   CRASH!                    │
│                           ╳   │   ╳                             │
│     (Both try to access shared data at the same time)          │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│  With mutex (traffic signal):                                   │
│                                                                 │
│     Thread A ────▶ [🔴 WAIT] │ ──▶ Thread B (has lock)         │
│                              │                                  │
│     Thread A (has lock) ──▶  │ [🔴 WAIT] ◀──── Thread B        │
│                              │                                  │
│     (One at a time, no crashes)                                │
└─────────────────────────────────────────────────────────────────┘
```

---

## Concept 4: Mutexes (Mutual Exclusion)

### The Traffic Signal Analogy

A **mutex** is like a traffic signal:
- Only ONE thread can hold the "lock" at a time
- Other threads must WAIT until the lock is released
- Prevents simultaneous access to shared data

### Basic Usage

```c
#include <pthread.h>

// Shared data and its protector
typedef struct {
    float x, y;
    pthread_mutex_t mutex;  // The "lock"
} SharedState;

SharedState shared;

// Initialize the mutex
pthread_mutex_init(&shared.mutex, NULL);

// Thread A (network):
void* network_func(void* arg) {
    while (running) {
        // Acquire lock (wait if someone else has it)
        pthread_mutex_lock(&shared.mutex);

        // CRITICAL SECTION: Only we can access shared now
        shared.x = received_data.x;
        shared.y = received_data.y;

        // Release lock (let others access)
        pthread_mutex_unlock(&shared.mutex);
    }
}

// Thread B (render):
void render(void) {
    pthread_mutex_lock(&shared.mutex);

    // Safe to read - no one else is writing
    DrawPlayer(shared.x, shared.y);

    pthread_mutex_unlock(&shared.mutex);
}

// When done
pthread_mutex_destroy(&shared.mutex);
```

### Critical Rules

1. **Always unlock what you lock** - forgetting to unlock = deadlock
2. **Lock for the shortest time possible** - long locks hurt performance
3. **Same lock order everywhere** - prevents deadlocks with multiple mutexes
4. **Initialize before use, destroy when done**

---

## Concept 5: Double Buffering for Game State

### The Pattern

Instead of locking for every read, use two buffers:

```c
typedef struct {
    PlayerState current;   // What render thread reads
    PlayerState next;      // What network thread writes to
    pthread_mutex_t mutex; // Protects the swap
} DoubleBufferedState;

// Network thread: Write to 'next'
void receive_state(DoubleBufferedState* state, PlayerState* new_data) {
    pthread_mutex_lock(&state->mutex);
    state->next = *new_data;  // Copy new data
    pthread_mutex_unlock(&state->mutex);
}

// Main thread: Swap buffers (once per frame)
void update_state(DoubleBufferedState* state) {
    pthread_mutex_lock(&state->mutex);
    state->current = state->next;  // Quick copy
    pthread_mutex_unlock(&state->mutex);
}

// Main thread: Read without locking (using 'current')
void render(DoubleBufferedState* state) {
    // No lock needed! 'current' only changes in update_state()
    // which runs on the same thread
    DrawPlayer(state->current.x, state->current.y);
}
```

### Benefits

- Network thread can write anytime
- Render thread reads without waiting
- Swap is fast (just a struct copy)
- No visual tearing

---

## Concept 6: Server-Authoritative Architecture

In our multiplayer game, the **server is the source of truth**:

```
┌─────────────────────────────────────────────────────────────────┐
│                 SERVER-AUTHORITATIVE MODEL                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   CLIENT A                 SERVER                  CLIENT B      │
│      │                       │                        │          │
│      │── Input (WASD) ──────▶│                        │          │
│      │                       │ Calculate physics      │          │
│      │                       │ Update positions       │          │
│      │                       │ Spawn bullets          │          │
│      │◀── Game State ────────│──── Game State ───────▶│          │
│      │   (positions,         │   (positions,          │          │
│      │    bullets)           │    bullets)            │          │
│      │                       │                        │          │
│   Display server             │               Display server      │
│   positions directly         │               positions directly  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘

Benefits:
  ✓ No cheating (clients can't lie about position)
  ✓ All clients see the same game state
  ✓ Simple - no prediction/reconciliation needed

Trade-off:
  ✗ Input latency (you see movement after server round-trip)
```

### Implementation in Code

```c
// In online mode, DON'T run local physics - use server position
if (!online) {
    update_local_player(&game.player, dt);  // Offline: local physics
} else {
    weapon_update(&game.player.weapon, dt); // Online: only update cooldowns
}

// Get authoritative position from server
if (online) {
    float server_x, server_y, server_vx, server_vy;
    if (shared_state_get_my_position(&shared, &server_x, &server_y,
                                      &server_vx, &server_vy)) {
        // Use server position directly - server is authoritative
        game.player.position.x = server_x;
        game.player.position.y = server_y;
    }
}
```

---

## Concept 7: Synchronized Weapons and Bullets

### Weapon Type Synchronization

Each client sends their current weapon type with every input:

```c
// Client sends weapon type so server knows fire rate/pattern
PlayerInputMsg input = {
    .player_id = my_id,
    .input_flags = flags,
    .weapon_type = player.weapon.type,  // 0=spread, 1=rapid, 2=laser
    .sequence = seq++
};
```

The server uses weapon-specific logic:

```c
// Server: Different weapons have different fire rates
float get_weapon_cooldown(uint8_t weapon_type) {
    switch (weapon_type) {
        case WEAPON_TYPE_SPREAD: return 1.0f / 3.0f;   // 3 shots/sec
        case WEAPON_TYPE_RAPID:  return 1.0f / 10.0f;  // 10 shots/sec
        case WEAPON_TYPE_LASER:  return 1.0f / 1.5f;   // 1.5 shots/sec
    }
}

// Server: Different weapons spawn different patterns
void server_spawn_bullet(GameServer* server, int player_id) {
    uint8_t weapon = server->players[player_id].weapon;

    switch (weapon) {
        case WEAPON_TYPE_SPREAD:
            // Spawn 3 bullets in a fan pattern (-15°, 0°, +15°)
            for (int i = 0; i < 3; i++) { /* ... */ }
            break;
        case WEAPON_TYPE_RAPID:
        case WEAPON_TYPE_LASER:
            // Single bullet straight up
            spawn_single_bullet(/* ... */);
            break;
    }
}
```

### Bullet Rendering by Weapon Type

Clients render remote bullets with weapon-appropriate visuals:

```c
void draw_remote_bullets(const GameState* game) {
    for (int i = 0; i < game->remote_bullet_count; i++) {
        RemoteBullet* rb = &game->remote_bullets[i];

        // Skip our own bullets (rendered locally for responsiveness)
        if (rb->owner_id == my_id) continue;

        // Weapon-specific colors and sizes
        switch (rb->weapon_type) {
            case WEAPON_TYPE_SPREAD:
                color = YELLOW_ORANGE;
                size = MEDIUM;
                break;
            case WEAPON_TYPE_RAPID:
                color = CYAN;
                size = SMALL;
                break;
            case WEAPON_TYPE_LASER:
                color = MAGENTA;
                size = LARGE;
                break;
        }

        DrawRectangle(rb->x, rb->y, size.w, size.h, color);
    }
}
```

---

## The Final Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    VOID DRIFTER FINAL                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │                     MAIN THREAD                         │  │
│   │                                                         │  │
│   │   Input ──▶ Set SharedState ──▶ Copy from SharedState  │  │
│   │     │            │                      │               │  │
│   │     │            │                      ▼               │  │
│   │     │            │              Apply server position   │  │
│   │     │            │                      │               │  │
│   │     │            │                      ▼               │  │
│   │     │            │                   Render             │  │
│   │     │            │                      │               │  │
│   │     ▼            ▼                      ▼               │  │
│   │   ┌───────────────────────────────────────────┐         │  │
│   │   │         SHARED STATE (Mutex Protected)    │         │  │
│   │   │   • input_to_send (flags + weapon_type)   │         │  │
│   │   │   • my_id (assigned by server)            │         │  │
│   │   │   • players[] (all player positions)      │         │  │
│   │   │   • bullets[] (all active bullets)        │         │  │
│   │   │   • connection status                     │         │  │
│   │   └───────────────────────────────────────────┘         │  │
│   │                         ▲                               │  │
│   └─────────────────────────┼───────────────────────────────┘  │
│                             │                                   │
│   ┌─────────────────────────┼───────────────────────────────┐  │
│   │                         │                               │  │
│   │              NETWORK THREAD                             │  │
│   │                                                         │  │
│   │   ┌─────────────────────────────────────────────────┐   │  │
│   │   │ HANDSHAKE (blocking - runs once)                │   │  │
│   │   │   1. TCP connect                                │   │  │
│   │   │   2. Send MSG_CONNECT                           │   │  │
│   │   │   3. Recv MSG_CONNECT_ACK                       │   │  │
│   │   │   4. Set socket non-blocking                    │   │  │
│   │   └─────────────────────────────────────────────────┘   │  │
│   │                         │                               │  │
│   │                         ▼                               │  │
│   │   ┌─────────────────────────────────────────────────┐   │  │
│   │   │ MAIN LOOP (non-blocking)                        │   │  │
│   │   │   • recv() game state (EAGAIN = no data, OK)    │   │  │
│   │   │   • Parse players[] and bullets[]               │   │  │
│   │   │   • Update SharedState                          │   │  │
│   │   │   • Get input from SharedState                  │   │  │
│   │   │   • send() MSG_PLAYER_INPUT                     │   │  │
│   │   │   • sleep(16ms)                                 │   │  │
│   │   └─────────────────────────────────────────────────┘   │  │
│   │                                                         │  │
│   └─────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## The Deliverable

The fully functional **Void Drifter** multiplayer game:

1. **Graphics** - Procedural textures, smooth rendering
2. **Weapons** - Multiple firing patterns via function pointers
3. **Networking** - Connect to server, receive state updates
4. **Threading** - Network thread runs independently
5. **No freezing** - Game stays responsive during network ops
6. **Server-Authoritative** - All players see consistent game state
7. **Weapon Sync** - Fire rates and patterns match across clients
8. **Bullet Sync** - All players see each other's bullets

### Multiplayer Features

| Feature | Implementation |
|---------|----------------|
| Movement | Server calculates, clients display |
| Weapons | Client sends type, server enforces fire rate |
| Bullets | Server spawns, broadcasts to all clients |
| Visuals | Weapon-specific colors/sizes for bullets |
| Disconnect | Graceful handling, other players continue |

```
┌─────────────────────────────────────────────────────────────────┐
│                        VOID DRIFTER                             │
│                                                                 │
│    ★           ● ● ●                            ★              │
│           ╔═══════════╗         ●                              │
│            \  |  /    ║         ●    ★                         │
│             \ | /     ║         ●                              │
│              \|/      ║                                        │
│               ▲ (You) ║              ▲ (Other Player)          │
│               █       ║              █                         │
│           [SPREAD]    ║          [RAPID]                       │
│                       ║                                        │
│  Yellow-orange        ║        Cyan bullets                    │
│  bullets (3 per shot) ║        (fast fire rate)                │
│                                                                │
│  Weapon: [SPREAD SHOT]   Health: 100   Network: Online        │
│  [1] Spread  [2] Rapid  [3] Laser     Players: 2              │
└─────────────────────────────────────────────────────────────────┘
```

### Testing Multiplayer

```bash
# Terminal 1: Start the server
cd module4_networking && ./server

# Terminal 2: Player 1
cd module5_concurrency && ./void_drifter --online

# Terminal 3: Player 2
cd module5_concurrency && ./void_drifter --online

# Test:
# - Move around: both players should see each other's movement
# - Switch weapons (1/2/3): other player sees different bullet patterns
# - Fire: other player sees your bullets with correct color/size/rate
# - Close one client: other client and server continue running
```

---

## File Structure

```
module5_concurrency/
├── main.c              # Entry point, game loop, rendering
│                       # - Handles input, sends to shared state
│                       # - Applies server position (online mode)
│                       # - Renders local and remote players/bullets
│
├── shared_state.h/c    # Thread-safe shared state with mutex
│                       # - RemotePlayer: other players' positions
│                       # - RemoteBullet: bullets from server
│                       # - Input queue: flags + weapon_type
│                       # - Connection status
│
├── network_client.h/c  # Network thread implementation
│                       # - Blocking handshake (MSG_CONNECT/ACK)
│                       # - Non-blocking game loop
│                       # - Receives players + bullets from server
│                       # - Sends input + weapon_type to server
│
├── protocol.h          # Shared with server (must match!)
│                       # - Message types and structs
│                       # - Weapon type constants
│                       # - Physics constants (speeds, rates)
│
├── weapon.h/c          # Weapon system (from Module 3)
│                       # - Fire functions per weapon type
│                       # - Cooldown management
│
├── bullet.h/c          # Local bullet system (from Module 3)
├── textures.h/c        # Procedural textures (from Module 2)
├── network.h/c         # Low-level socket helpers
└── Makefile
```

### Module 4 Server (Required for Online Play)

```
module4_networking/
├── server.c            # Authoritative game server
│                       # - Accepts connections (reads MSG_CONNECT)
│                       # - Tracks all players and bullets
│                       # - Weapon-specific fire rates/patterns
│                       # - Broadcasts game state to all clients
│                       # - Handles SIGPIPE gracefully
│
├── protocol.h          # Must match module5 version!
├── network.h/c         # Socket helpers
└── Makefile
```

Let's build the final game!
