/**
 * shared_state.c - Thread-Safe Shared State Implementation
 *
 * This file implements all the mutex-protected operations on shared state.
 *
 * PATTERN: Lock-Modify-Unlock
 * ===========================
 * Every function that touches shared data follows this pattern:
 *     1. pthread_mutex_lock(&state->mutex)
 *     2. Read or modify the data
 *     3. pthread_mutex_unlock(&state->mutex)
 *
 * This ensures only one thread can access the data at a time.
 */

#include "shared_state.h"
#include <string.h>
#include <stdio.h>

/**
 * shared_state_init - Initialize the shared state
 */
int shared_state_init(SharedState* state) {
    if (state == NULL) return -1;

    // Zero out all data
    memset(state, 0, sizeof(SharedState));

    // Initialize the mutex
    // NULL = default attributes
    if (pthread_mutex_init(&state->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex\n");
        return -1;
    }

    // Set initial status
    state->status = NET_DISCONNECTED;
    snprintf(state->status_message, sizeof(state->status_message), "Not connected");

    // Initialize player slots
    for (int i = 0; i < MAX_PLAYERS; i++) {
        state->players[i].active = 0;
    }

    return 0;
}

/**
 * shared_state_destroy - Clean up the shared state
 *
 * IMPORTANT: Only call this after all threads have stopped!
 * Destroying a mutex while another thread holds it = undefined behavior.
 */
void shared_state_destroy(SharedState* state) {
    if (state == NULL) return;

    // Destroy the mutex
    pthread_mutex_destroy(&state->mutex);
}

/**
 * shared_state_lock - Acquire the mutex
 *
 * If another thread holds the lock, this call BLOCKS until it's released.
 */
void shared_state_lock(SharedState* state) {
    pthread_mutex_lock(&state->mutex);
}

/**
 * shared_state_unlock - Release the mutex
 *
 * CRITICAL: Always call this after lock, even if an error occurs!
 * Use the pattern:
 *     lock();
 *     // ... code that might return early ...
 *     unlock();
 *
 * Or better, don't return early while holding a lock.
 */
void shared_state_unlock(SharedState* state) {
    pthread_mutex_unlock(&state->mutex);
}

/**
 * shared_state_set_status - Update connection status
 */
void shared_state_set_status(SharedState* state, NetworkStatus status, const char* message) {
    if (state == NULL) return;

    pthread_mutex_lock(&state->mutex);

    state->status = status;
    if (message != NULL) {
        strncpy(state->status_message, message, sizeof(state->status_message) - 1);
        state->status_message[sizeof(state->status_message) - 1] = '\0';
    }

    pthread_mutex_unlock(&state->mutex);
}

/**
 * shared_state_get_status - Get current connection status
 */
NetworkStatus shared_state_get_status(SharedState* state) {
    if (state == NULL) return NET_DISCONNECTED;

    pthread_mutex_lock(&state->mutex);
    NetworkStatus status = state->status;
    pthread_mutex_unlock(&state->mutex);

    return status;
}

/**
 * shared_state_set_input - Set input to be sent to server
 *
 * Called by main thread after gathering keyboard input.
 */
void shared_state_set_input(SharedState* state, uint8_t input_flags, uint8_t weapon_type) {
    if (state == NULL) return;

    pthread_mutex_lock(&state->mutex);

    state->input_to_send = input_flags;
    state->weapon_type = weapon_type;
    state->input_sequence++;

    pthread_mutex_unlock(&state->mutex);
}

/**
 * shared_state_get_input - Get pending input for network send
 *
 * Called by network thread to get input to send to server.
 */
uint8_t shared_state_get_input(SharedState* state, uint32_t* sequence, uint8_t* weapon_type) {
    if (state == NULL) return 0;

    pthread_mutex_lock(&state->mutex);

    uint8_t flags = state->input_to_send;
    if (sequence != NULL) {
        *sequence = state->input_sequence;
    }
    if (weapon_type != NULL) {
        *weapon_type = state->weapon_type;
    }

    pthread_mutex_unlock(&state->mutex);

    return flags;
}

/**
 * shared_state_update_players - Update player data from server
 *
 * Called by network thread when we receive a game state update.
 *
 * CONCEPT: Atomic Update
 * ======================
 * We update ALL player data while holding the lock, so the main thread
 * never sees a partially updated state.
 */
void shared_state_update_players(SharedState* state, const RemotePlayer* players,
                                  int count, uint32_t server_tick) {
    if (state == NULL || players == NULL) return;

    pthread_mutex_lock(&state->mutex);

    // Clear all players first
    for (int i = 0; i < MAX_PLAYERS; i++) {
        state->players[i].active = 0;
    }

    // Copy new player data
    int copied = (count > MAX_PLAYERS) ? MAX_PLAYERS : count;
    for (int i = 0; i < copied; i++) {
        state->players[i] = players[i];
        state->players[i].active = 1;
    }

    state->player_count = copied;
    state->server_tick = server_tick;
    state->packets_received++;

    pthread_mutex_unlock(&state->mutex);
}

/**
 * shared_state_copy_players - Copy player data for rendering
 *
 * PATTERN: Copy Under Lock
 * ========================
 * Instead of returning pointers to shared data (dangerous!),
 * we COPY the data to a caller-provided buffer.
 *
 * This way:
 *     1. Caller has their own copy (can read without lock)
 *     2. Lock is held briefly (just for memcpy)
 *     3. No risk of stale pointers
 */
int shared_state_copy_players(SharedState* state, RemotePlayer* out) {
    if (state == NULL || out == NULL) return 0;

    pthread_mutex_lock(&state->mutex);

    // Copy all player data
    memcpy(out, state->players, sizeof(state->players));
    int count = state->player_count;

    pthread_mutex_unlock(&state->mutex);

    return count;
}

/**
 * shared_state_get_my_position - Get local player's server-authoritative position
 *
 * Finds our player in the players array and returns their position.
 * Used for position reconciliation - the server is the authority.
 */
int shared_state_get_my_position(SharedState* state, float* x, float* y, float* vx, float* vy) {
    if (state == NULL) return 0;

    pthread_mutex_lock(&state->mutex);

    int found = 0;
    uint8_t my_id = state->my_id;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->players[i].active && state->players[i].id == my_id) {
            if (x)  *x  = state->players[i].x;
            if (y)  *y  = state->players[i].y;
            if (vx) *vx = state->players[i].vx;
            if (vy) *vy = state->players[i].vy;
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&state->mutex);

    return found;
}

/**
 * shared_state_update_bullets - Update bullet data from server
 */
void shared_state_update_bullets(SharedState* state, const RemoteBullet* bullets, int count) {
    if (state == NULL || bullets == NULL) return;

    pthread_mutex_lock(&state->mutex);

    // Clear existing bullets
    for (int i = 0; i < MAX_REMOTE_BULLETS; i++) {
        state->bullets[i].active = 0;
    }

    // Copy new bullet data
    int copied = (count > MAX_REMOTE_BULLETS) ? MAX_REMOTE_BULLETS : count;
    for (int i = 0; i < copied; i++) {
        state->bullets[i] = bullets[i];
        state->bullets[i].active = 1;
    }
    state->bullet_count = copied;

    pthread_mutex_unlock(&state->mutex);
}

/**
 * shared_state_copy_bullets - Copy bullet data for rendering
 */
int shared_state_copy_bullets(SharedState* state, RemoteBullet* out) {
    if (state == NULL || out == NULL) return 0;

    pthread_mutex_lock(&state->mutex);

    memcpy(out, state->bullets, sizeof(state->bullets));
    int count = state->bullet_count;

    pthread_mutex_unlock(&state->mutex);

    return count;
}
