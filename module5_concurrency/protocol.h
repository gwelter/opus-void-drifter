/**
 * protocol.h - Network Protocol Definitions
 *
 * CONCEPT: Shared Protocol
 * ========================
 * Both client and server include this file to ensure they agree on:
 *     - Message types
 *     - Data structures
 *     - Byte layout
 *
 * This is like a TypeScript interface that both sides implement.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>  // For fixed-width types (uint8_t, uint16_t, etc.)

/**
 * CONCEPT: Fixed-Width Integer Types
 * ==================================
 * In C, 'int' can be different sizes on different systems:
 *     - 16-bit systems: int = 2 bytes
 *     - 32-bit systems: int = 4 bytes
 *     - Some 64-bit:    int = 8 bytes
 *
 * For network protocols, we need EXACT sizes:
 *     uint8_t  = Unsigned 8-bit  (1 byte, 0-255)
 *     uint16_t = Unsigned 16-bit (2 bytes, 0-65535)
 *     uint32_t = Unsigned 32-bit (4 bytes, 0-4 billion)
 *     int32_t  = Signed 32-bit   (4 bytes, -2B to +2B)
 */

// Network configuration
#define DEFAULT_PORT 8080
#define MAX_CLIENTS 4
#define BUFFER_SIZE 1024

/**
 * MessageType - Types of messages in our protocol
 *
 * CONCEPT: Protocol Versioning
 * ============================
 * In a real game, you'd include a version number to handle
 * clients and servers running different versions.
 */
typedef enum {
    MSG_NONE = 0,         // Invalid/empty message
    MSG_CONNECT,          // Client wants to join
    MSG_CONNECT_ACK,      // Server acknowledges connection
    MSG_DISCONNECT,       // Either side is leaving
    MSG_PLAYER_INPUT,     // Client sends input state
    MSG_GAME_STATE,       // Server sends world state
    MSG_PING,             // Latency check request
    MSG_PONG              // Latency check response
} MessageType;

/**
 * CONCEPT: __attribute__((packed))
 * ================================
 * By default, the compiler adds padding bytes between struct members
 * for CPU alignment optimization. This is a problem for networking:
 *
 * Without packed:
 *     struct { char a; int b; }  // May be 8 bytes (1 + 3 padding + 4)
 *
 * With packed:
 *     struct __attribute__((packed)) { char a; int b; }  // Exactly 5 bytes
 *
 * TRADE-OFF:
 *     - Packed structs are portable across different machines
 *     - But may be slower to access on some architectures
 *     - Use packed ONLY for network messages, not for game state
 */

/**
 * MessageHeader - Fixed header at the start of every message
 *
 * Layout (4 bytes total):
 * ┌──────────┬──────────────────┐
 * │   type   │     length       │
 * │  1 byte  │     2 bytes      │
 * └──────────┴──────────────────┘
 *
 * The 'length' field tells us how many more bytes to read
 * after the header.
 */
typedef struct __attribute__((packed)) {
    uint8_t type;       // MessageType enum value
    uint16_t length;    // Length of payload after this header
} MessageHeader;

/**
 * InputFlags - Bitfield for player input
 *
 * CONCEPT: Bitfields for Efficiency
 * =================================
 * Instead of 5 separate bools (5 bytes minimum), we pack
 * 5 flags into a single byte using bit manipulation:
 *
 *     byte: 0 0 0 F R D L U
 *           │ │ │ │ │ │ │ └─ UP pressed
 *           │ │ │ │ │ │ └─── LEFT pressed
 *           │ │ │ │ │ └───── DOWN pressed
 *           │ │ │ │ └─────── RIGHT pressed
 *           │ │ │ └───────── FIRE pressed
 *           └─┴─┴─────────── Reserved
 *
 * To check if UP is pressed:
 *     if (flags & INPUT_UP) { ... }
 *
 * To set FIRE:
 *     flags |= INPUT_FIRE;
 */
#define INPUT_UP    (1 << 0)  // 0b00000001
#define INPUT_LEFT  (1 << 1)  // 0b00000010
#define INPUT_DOWN  (1 << 2)  // 0b00000100
#define INPUT_RIGHT (1 << 3)  // 0b00001000
#define INPUT_FIRE  (1 << 4)  // 0b00010000

/**
 * PlayerInputMsg - Client tells server what keys are pressed
 *
 * Sent from Client -> Server every frame (or when input changes)
 */
typedef struct __attribute__((packed)) {
    uint8_t player_id;   // Which player (for future multiplayer)
    uint8_t input_flags; // Bitfield of INPUT_* flags
    uint8_t weapon_type; // Current weapon (0=spread, 1=rapid, 2=laser)
    uint32_t sequence;   // Message sequence number (for ordering)
} PlayerInputMsg;

// Weapon types (must match client's WeaponType enum)
#define WEAPON_TYPE_SPREAD 0
#define WEAPON_TYPE_RAPID  1
#define WEAPON_TYPE_LASER  2

/**
 * PlayerState - Snapshot of a single player
 *
 * Part of the game state sent from server to client.
 */
typedef struct __attribute__((packed)) {
    uint8_t player_id;   // Player identifier
    float x, y;          // Position
    float vx, vy;        // Velocity
    int16_t health;      // Current health
    uint8_t weapon;      // Current weapon type
    uint8_t flags;       // State flags (alive, firing, etc.)
} PlayerState;

/**
 * BulletState - Snapshot of a single bullet
 *
 * Part of the game state sent from server to client.
 */
typedef struct __attribute__((packed)) {
    uint8_t owner_id;        // Which player fired this bullet
    float x, y;              // Position
    float vx, vy;            // Velocity
    uint8_t weapon_type;     // Type of weapon that created it
} BulletState;

// Maximum bullets to sync per frame
#define MAX_SYNC_BULLETS 50

/**
 * GameStateMsg - Server sends current world state to client
 *
 * Contains positions of all players and bullets.
 */
typedef struct __attribute__((packed)) {
    uint32_t tick;           // Server tick number
    uint32_t your_sequence;  // Last input sequence server processed
    uint8_t player_count;    // Number of players in this update
    uint8_t bullet_count;    // Number of bullets in this update
    // Followed by player_count * PlayerState, then bullet_count * BulletState
    // We use a flexible array member for players (bullets follow after)
    PlayerState players[];   // Array of player states (C99 flexible array)
} GameStateMsg;

/**
 * ConnectMsg - Client requests to join the game
 */
typedef struct __attribute__((packed)) {
    uint8_t version;         // Protocol version
    char name[16];           // Player name (null-terminated)
} ConnectMsg;

/**
 * ConnectAckMsg - Server acknowledges connection
 */
typedef struct __attribute__((packed)) {
    uint8_t success;         // 1 = connected, 0 = rejected
    uint8_t player_id;       // Assigned player ID
    uint8_t reason;          // If rejected, why (0 = full, 1 = version mismatch)
} ConnectAckMsg;

/**
 * PingMsg / PongMsg - Latency measurement
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;      // Client's timestamp (for RTT calculation)
} PingMsg;

typedef struct __attribute__((packed)) {
    uint32_t client_timestamp;  // Echo back client's timestamp
    uint32_t server_timestamp;  // Server's current timestamp
} PongMsg;

/**
 * Helper macros for message size calculation
 *
 * CONCEPT: sizeof with Flexible Arrays
 * ====================================
 * GameStateMsg has a flexible array member (players[]).
 * sizeof(GameStateMsg) gives the size WITHOUT any array elements.
 * To get total size: sizeof(GameStateMsg) + count * sizeof(PlayerState)
 */
#define MSG_SIZE_CONNECT        (sizeof(MessageHeader) + sizeof(ConnectMsg))
#define MSG_SIZE_CONNECT_ACK    (sizeof(MessageHeader) + sizeof(ConnectAckMsg))
#define MSG_SIZE_PLAYER_INPUT   (sizeof(MessageHeader) + sizeof(PlayerInputMsg))
#define MSG_SIZE_PING           (sizeof(MessageHeader) + sizeof(PingMsg))
#define MSG_SIZE_PONG           (sizeof(MessageHeader) + sizeof(PongMsg))
#define MSG_SIZE_GAME_STATE(n)  (sizeof(MessageHeader) + sizeof(GameStateMsg) + (n) * sizeof(PlayerState))

// Protocol version (increment when making breaking changes)
#define PROTOCOL_VERSION 1

/**
 * Shared Physics Constants
 * ========================
 * These constants MUST be identical on client and server to ensure
 * consistent movement between what the client predicts and what the
 * server calculates.
 */
#define PLAYER_SPEED        300.0f    // Max velocity
#define PLAYER_ACCELERATION 800.0f    // How fast we reach max speed
#define PLAYER_FRICTION     0.95f     // Velocity multiplier per frame (at 60fps)

// Screen bounds (for position clamping)
#define GAME_WIDTH  800
#define GAME_HEIGHT 600

#endif // PROTOCOL_H
