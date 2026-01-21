/**
 * shared_state.h - Thread-Safe Shared State
 *
 * CONCEPT: Data Synchronization
 * =============================
 * When multiple threads access the same data, we need synchronization.
 * This module provides a mutex-protected state container that both
 * the main thread and network thread can safely access.
 *
 * The pattern used here is "double buffering":
 *     - Network thread writes to 'server_state'
 *     - Main thread reads from 'local_state'
 *     - Once per frame, we copy server -> local (under lock)
 */

#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <pthread.h>
#include <stdint.h>
#include "raylib.h"

// Maximum players supported
#define MAX_PLAYERS 4

// Maximum bullets to track
#define MAX_REMOTE_BULLETS 50

/**
 * RemotePlayer - State of another player received from server
 */
typedef struct {
    int active;
    uint8_t id;
    float x, y;
    float vx, vy;
    int health;
    uint8_t weapon;
} RemotePlayer;

/**
 * RemoteBullet - State of a bullet received from server
 */
typedef struct {
    int active;
    uint8_t owner_id;
    float x, y;
    float vx, vy;
    uint8_t weapon_type;
} RemoteBullet;

/**
 * NetworkStatus - Connection state
 */
typedef enum {
    NET_DISCONNECTED,
    NET_CONNECTING,
    NET_CONNECTED,
    NET_ERROR
} NetworkStatus;

/**
 * SharedState - Data shared between main and network threads
 *
 * CRITICAL: All access must be protected by the mutex!
 *
 * Fields:
 *     - mutex: The lock that protects all other fields
 *     - status: Current network connection status
 *     - my_id: Our player ID assigned by server
 *     - players: State of all players from server
 *     - server_tick: Last tick number from server
 *     - input_to_send: Input flags to be sent to server
 */
typedef struct {
    // Synchronization
    pthread_mutex_t mutex;

    // Network status
    NetworkStatus status;
    char status_message[64];

    // Our identity
    uint8_t my_id;

    // Server-authoritative state
    RemotePlayer players[MAX_PLAYERS];
    int player_count;
    uint32_t server_tick;

    // Bullets from server
    RemoteBullet bullets[MAX_REMOTE_BULLETS];
    int bullet_count;

    // Client -> Server communication
    uint8_t input_to_send;      // Input flags to send next
    uint8_t weapon_type;        // Current weapon type
    uint32_t input_sequence;    // Sequence number

    // Statistics
    float ping_ms;              // Round-trip time
    int packets_received;
    int packets_sent;

} SharedState;

/**
 * shared_state_init - Initialize the shared state
 *
 * IMPORTANT: Must be called before any threads are created!
 *
 * @param state  State to initialize
 * @return       0 on success, -1 on failure
 */
int shared_state_init(SharedState* state);

/**
 * shared_state_destroy - Clean up the shared state
 *
 * IMPORTANT: Call after all threads have finished!
 *
 * @param state  State to destroy
 */
void shared_state_destroy(SharedState* state);

/**
 * shared_state_lock - Acquire the mutex
 *
 * Call this before reading or writing any shared data.
 * Don't forget to call unlock!
 *
 * @param state  State to lock
 */
void shared_state_lock(SharedState* state);

/**
 * shared_state_unlock - Release the mutex
 *
 * Call this after you're done with shared data.
 *
 * @param state  State to unlock
 */
void shared_state_unlock(SharedState* state);

/**
 * shared_state_set_status - Update connection status (thread-safe)
 *
 * @param state    State to update
 * @param status   New status
 * @param message  Status message (will be copied)
 */
void shared_state_set_status(SharedState* state, NetworkStatus status, const char* message);

/**
 * shared_state_get_status - Get current connection status (thread-safe)
 *
 * @param state  State to query
 * @return       Current status
 */
NetworkStatus shared_state_get_status(SharedState* state);

/**
 * shared_state_set_input - Set input to be sent to server (thread-safe)
 *
 * Called by main thread after processing input.
 *
 * @param state        State to update
 * @param input_flags  The input flags to send
 * @param weapon_type  Current weapon type
 */
void shared_state_set_input(SharedState* state, uint8_t input_flags, uint8_t weapon_type);

/**
 * shared_state_get_input - Get pending input (thread-safe)
 *
 * Called by network thread to get input to send.
 *
 * @param state       State to query
 * @param sequence    Output: The sequence number
 * @param weapon_type Output: Current weapon type
 * @return            Input flags
 */
uint8_t shared_state_get_input(SharedState* state, uint32_t* sequence, uint8_t* weapon_type);

/**
 * shared_state_update_players - Update player data from server (thread-safe)
 *
 * Called by network thread when server state arrives.
 *
 * @param state        State to update
 * @param players      Array of player data
 * @param count        Number of players
 * @param server_tick  Server's tick number
 */
void shared_state_update_players(SharedState* state, const RemotePlayer* players,
                                  int count, uint32_t server_tick);

/**
 * shared_state_copy_players - Copy player data for rendering (thread-safe)
 *
 * Called by main thread to get current player positions.
 *
 * @param state   State to read from
 * @param out     Output array (must be at least MAX_PLAYERS)
 * @return        Number of players copied
 */
int shared_state_copy_players(SharedState* state, RemotePlayer* out);

/**
 * shared_state_get_my_position - Get local player's server-authoritative position
 *
 * Used for position reconciliation in online mode.
 *
 * @param state   State to read from
 * @param x       Output: player X position
 * @param y       Output: player Y position
 * @param vx      Output: player X velocity
 * @param vy      Output: player Y velocity
 * @return        1 if found, 0 if not connected or no data yet
 */
int shared_state_get_my_position(SharedState* state, float* x, float* y, float* vx, float* vy);

/**
 * shared_state_update_bullets - Update bullet data from server (thread-safe)
 *
 * Called by network thread when server state arrives.
 *
 * @param state    State to update
 * @param bullets  Array of bullet data
 * @param count    Number of bullets
 */
void shared_state_update_bullets(SharedState* state, const RemoteBullet* bullets, int count);

/**
 * shared_state_copy_bullets - Copy bullet data for rendering (thread-safe)
 *
 * Called by main thread to get current bullet positions.
 *
 * @param state   State to read from
 * @param out     Output array (must be at least MAX_REMOTE_BULLETS)
 * @return        Number of bullets copied
 */
int shared_state_copy_bullets(SharedState* state, RemoteBullet* out);

#endif // SHARED_STATE_H
