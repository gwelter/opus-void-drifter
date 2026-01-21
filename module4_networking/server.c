/**
 * server.c - Void Drifter Game Server
 *
 * This is a simple authoritative game server that:
 *     1. Listens for client connections
 *     2. Receives player input
 *     3. Updates game state
 *     4. Sends state back to clients
 *
 * ARCHITECTURE: Authoritative Server
 * ==================================
 * The server is the "source of truth":
 *     - Clients send input (what buttons they're pressing)
 *     - Server calculates the result (where player moves)
 *     - Server sends the result back (your new position)
 *
 * This prevents cheating (clients can't just say "I'm at the exit!").
 *
 * NOTE: This is a BLOCKING server for educational purposes.
 * In Module 5, we'll make it non-blocking with threads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>

#include "protocol.h"
#include "network.h"

// Server configuration
#define SERVER_PORT 8080
#define TICK_RATE 60        // Updates per second
#define MAX_PLAYERS 4

// Global running flag (for signal handling)
static volatile int g_running = 1;

/**
 * ServerPlayer - Server's view of a connected player
 */
typedef struct {
    int active;             // Is this slot in use?
    Socket socket;          // Client's socket
    char name[16];          // Player name
    struct sockaddr_in addr; // Client address

    // Game state (server is authoritative)
    float x, y;             // Position
    float vx, vy;           // Velocity
    int health;             // HP
    uint8_t weapon;         // Current weapon
    uint8_t input_flags;    // Last received input
    uint32_t last_sequence; // Last input sequence number

    // Weapon state
    float fire_cooldown;    // Time until can fire again
} ServerPlayer;

/**
 * ServerBullet - A bullet in the game world
 */
typedef struct {
    int active;
    uint8_t owner_id;       // Which player fired it
    float x, y;             // Position
    float vx, vy;           // Velocity
    uint8_t weapon_type;    // Type of weapon that created it
    float lifetime;         // Remaining lifetime
} ServerBullet;

// Maximum bullets the server tracks
#define MAX_SERVER_BULLETS 200

// Bullet configuration
#define BULLET_LIFETIME 2.0f

// Weapon-specific configurations (must match client weapon.c)
// Fire rates: shots per second -> cooldown = 1/rate
#define SPREAD_FIRE_RATE    3.0f    // 3 shots/sec
#define RAPID_FIRE_RATE     10.0f   // 10 shots/sec
#define LASER_FIRE_RATE     1.5f    // 1.5 shots/sec

#define SPREAD_BULLET_SPEED 400.0f
#define RAPID_BULLET_SPEED  600.0f
#define LASER_BULLET_SPEED  800.0f

/**
 * GameServer - Server state
 */
typedef struct {
    Socket listen_socket;
    ServerPlayer players[MAX_PLAYERS];
    int player_count;
    uint32_t tick;          // Server tick counter

    // Bullets
    ServerBullet bullets[MAX_SERVER_BULLETS];
    int bullet_count;
} GameServer;

/**
 * signal_handler - Handle Ctrl+C gracefully
 */
static void signal_handler(int sig) {
    (void)sig;
    printf("\nShutting down server...\n");
    g_running = 0;
}

/**
 * server_init - Initialize the game server
 */
static int server_init(GameServer* server, uint16_t port) {
    printf("Initializing server on port %d...\n", port);

    // Initialize player slots
    memset(server->players, 0, sizeof(server->players));
    server->player_count = 0;
    server->tick = 0;

    // Create listening socket
    server->listen_socket = net_create_server(port, 5);
    if (server->listen_socket == INVALID_SOCKET) {
        fprintf(stderr, "Failed to create server socket\n");
        return -1;
    }

    printf("Server listening on port %d\n", port);
    return 0;
}

/**
 * server_cleanup - Clean up server resources
 */
static void server_cleanup(GameServer* server) {
    // Close all client connections
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server->players[i].active) {
            net_close(server->players[i].socket);
        }
    }

    // Close listening socket
    net_close(server->listen_socket);
    printf("Server cleaned up\n");
}

/**
 * server_find_free_slot - Find an unused player slot
 */
static int server_find_free_slot(GameServer* server) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!server->players[i].active) {
            return i;
        }
    }
    return -1;  // No free slots
}

/**
 * server_accept_new_client - Handle a new client connection
 *
 * Protocol:
 *   1. Accept TCP connection
 *   2. Read MSG_CONNECT from client
 *   3. Validate and send MSG_CONNECT_ACK
 *   4. Initialize player if successful
 */
static void server_accept_new_client(GameServer* server) {
    struct sockaddr_in client_addr;
    Socket client_socket = net_accept_client(server->listen_socket, &client_addr);

    if (client_socket == INVALID_SOCKET) {
        return;  // No client waiting
    }

    char addr_str[32];
    net_addr_to_string(&client_addr, addr_str, sizeof(addr_str));
    printf("New connection from %s\n", addr_str);

    // --- READ MSG_CONNECT FROM CLIENT ---
    // The client sends MSG_CONNECT after TCP connect, we must read it
    MessageHeader connect_header;
    if (net_recv_all(client_socket, &connect_header, sizeof(connect_header)) != sizeof(connect_header)) {
        printf("Failed to read connect header from %s\n", addr_str);
        net_close(client_socket);
        return;
    }

    if (connect_header.type != MSG_CONNECT) {
        printf("Expected MSG_CONNECT, got type %d from %s\n", connect_header.type, addr_str);
        net_close(client_socket);
        return;
    }

    ConnectMsg connect_msg;
    if (net_recv_all(client_socket, &connect_msg, sizeof(connect_msg)) != sizeof(connect_msg)) {
        printf("Failed to read connect payload from %s\n", addr_str);
        net_close(client_socket);
        return;
    }

    // Check protocol version
    if (connect_msg.version != PROTOCOL_VERSION) {
        printf("Version mismatch from %s (got %d, expected %d)\n",
               addr_str, connect_msg.version, PROTOCOL_VERSION);
        ConnectAckMsg ack = { .success = 0, .player_id = 0, .reason = 1 };
        MessageHeader header = { .type = MSG_CONNECT_ACK, .length = sizeof(ack) };
        net_send_all(client_socket, &header, sizeof(header));
        net_send_all(client_socket, &ack, sizeof(ack));
        net_close(client_socket);
        return;
    }

    // Find a free player slot
    int slot = server_find_free_slot(server);
    if (slot < 0) {
        printf("Server full, rejecting connection from %s\n", addr_str);
        ConnectAckMsg ack = { .success = 0, .player_id = 0, .reason = 0 };
        MessageHeader header = { .type = MSG_CONNECT_ACK, .length = sizeof(ack) };
        net_send_all(client_socket, &header, sizeof(header));
        net_send_all(client_socket, &ack, sizeof(ack));
        net_close(client_socket);
        return;
    }

    // Initialize player
    ServerPlayer* player = &server->players[slot];
    memset(player, 0, sizeof(ServerPlayer));
    player->active = 1;
    player->socket = client_socket;
    player->addr = client_addr;
    // Use name from connect message if provided, otherwise default
    if (connect_msg.name[0] != '\0') {
        strncpy(player->name, connect_msg.name, sizeof(player->name) - 1);
        player->name[sizeof(player->name) - 1] = '\0';
    } else {
        snprintf(player->name, sizeof(player->name), "Player%d", slot + 1);
    }

    // Initial position (spread players out)
    player->x = 100.0f + (slot * 150.0f);
    player->y = 400.0f;
    player->health = 100;
    player->weapon = 0;

    server->player_count++;

    // Send acceptance message
    ConnectAckMsg ack = {
        .success = 1,
        .player_id = (uint8_t)slot,
        .reason = 0
    };
    MessageHeader header = { .type = MSG_CONNECT_ACK, .length = sizeof(ack) };
    net_send_all(client_socket, &header, sizeof(header));
    net_send_all(client_socket, &ack, sizeof(ack));

    printf("Player %d (%s) joined from %s\n", slot, player->name, addr_str);
}

/**
 * server_handle_client_message - Process a message from a client
 *
 * NOTE: Uses non-blocking sockets, so we must distinguish between:
 *   - No data available (EAGAIN) -> just return, try again later
 *   - Connection closed (recv returns 0 with data) -> disconnect
 *   - Error -> disconnect
 */
static void server_handle_client_message(GameServer* server, int player_id) {
    ServerPlayer* player = &server->players[player_id];
    if (!player->active) return;

    // Read message header (non-blocking)
    MessageHeader header;
    int bytes = recv(player->socket, &header, sizeof(header), 0);

    if (bytes == 0) {
        // Connection closed by client
        printf("Player %d disconnected (connection closed)\n", player_id);
        net_close(player->socket);
        player->active = 0;
        server->player_count--;
        return;
    }

    if (bytes < 0) {
        // Check if it's just "no data available" vs actual error
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available right now, that's fine
            return;
        }
        // Actual error
        printf("Player %d disconnected (error: %s)\n", player_id, strerror(errno));
        net_close(player->socket);
        player->active = 0;
        server->player_count--;
        return;
    }

    if (bytes != sizeof(header)) {
        // Partial header - shouldn't happen with small reads, but handle it
        return;
    }

    // Handle message based on type
    switch (header.type) {
        case MSG_PLAYER_INPUT: {
            // Read input data
            PlayerInputMsg input;
            if (net_recv_all(player->socket, &input, sizeof(input)) <= 0) {
                return;
            }

            // Validate sequence (ignore old/duplicate messages)
            if (input.sequence <= player->last_sequence) {
                return;  // Old message, ignore
            }
            player->last_sequence = input.sequence;

            // Store input for processing in update
            player->input_flags = input.input_flags;
            player->weapon = input.weapon_type;

            // Debug output (reduced verbosity - only show changes)
            static uint8_t last_flags[MAX_PLAYERS] = {0};
            if (input.input_flags != last_flags[player_id]) {
                printf("Player %d input: ", player_id);
                if (input.input_flags & INPUT_UP) printf("UP ");
                if (input.input_flags & INPUT_DOWN) printf("DOWN ");
                if (input.input_flags & INPUT_LEFT) printf("LEFT ");
                if (input.input_flags & INPUT_RIGHT) printf("RIGHT ");
                if (input.input_flags & INPUT_FIRE) printf("FIRE ");
                printf("weapon=%d\n", input.weapon_type);
                last_flags[player_id] = input.input_flags;
            }
            printf("(seq=%u)\n", input.sequence);
            break;
        }

        case MSG_DISCONNECT: {
            printf("Player %d sent disconnect\n", player_id);
            net_close(player->socket);
            player->active = 0;
            server->player_count--;
            break;
        }

        case MSG_PING: {
            // Echo back pong
            PingMsg ping;
            if (net_recv_all(player->socket, &ping, sizeof(ping)) > 0) {
                PongMsg pong = {
                    .client_timestamp = ping.timestamp,
                    .server_timestamp = server->tick
                };
                MessageHeader pong_header = { .type = MSG_PONG, .length = sizeof(pong) };
                net_send_all(player->socket, &pong_header, sizeof(pong_header));
                net_send_all(player->socket, &pong, sizeof(pong));
            }
            break;
        }

        default:
            printf("Unknown message type %d from player %d\n", header.type, player_id);
            break;
    }
}

/**
 * server_find_free_bullet - Find an unused bullet slot
 */
static int server_find_free_bullet(GameServer* server) {
    for (int i = 0; i < MAX_SERVER_BULLETS; i++) {
        if (!server->bullets[i].active) {
            return i;
        }
    }
    return -1;  // No free slots
}

/**
 * server_spawn_single_bullet - Create a single bullet with given parameters
 */
static void server_spawn_single_bullet(GameServer* server, int player_id,
                                        float x, float y, float vx, float vy,
                                        uint8_t weapon_type) {
    int slot = server_find_free_bullet(server);
    if (slot < 0) return;  // No room for more bullets

    ServerBullet* bullet = &server->bullets[slot];
    bullet->active = 1;
    bullet->owner_id = (uint8_t)player_id;
    bullet->x = x;
    bullet->y = y;
    bullet->vx = vx;
    bullet->vy = vy;
    bullet->weapon_type = weapon_type;
    bullet->lifetime = BULLET_LIFETIME;

    server->bullet_count++;
}

/**
 * server_spawn_bullet - Create bullets based on weapon type
 */
static void server_spawn_bullet(GameServer* server, int player_id, float x, float y) {
    uint8_t weapon = server->players[player_id].weapon;

    switch (weapon) {
        case WEAPON_TYPE_SPREAD: {
            // Spread: 3 bullets in a fan pattern
            float speed = SPREAD_BULLET_SPEED;
            float angles[] = { -0.2618f, 0.0f, 0.2618f };  // -15, 0, +15 degrees in radians

            for (int i = 0; i < 3; i++) {
                float vx = speed * sinf(angles[i]);
                float vy = -speed * cosf(angles[i]);
                float spawn_x = x + 10.0f * sinf(angles[i]);
                float spawn_y = y - 20.0f;
                server_spawn_single_bullet(server, player_id, spawn_x, spawn_y, vx, vy, weapon);
            }
            break;
        }

        case WEAPON_TYPE_RAPID: {
            // Rapid: single fast bullet straight up
            float speed = RAPID_BULLET_SPEED;
            server_spawn_single_bullet(server, player_id, x, y - 25.0f, 0, -speed, weapon);
            break;
        }

        case WEAPON_TYPE_LASER: {
            // Laser: single very fast bullet
            float speed = LASER_BULLET_SPEED;
            server_spawn_single_bullet(server, player_id, x, y - 30.0f, 0, -speed, weapon);
            break;
        }

        default: {
            // Default to spread behavior
            float speed = SPREAD_BULLET_SPEED;
            server_spawn_single_bullet(server, player_id, x, y - 20.0f, 0, -speed, weapon);
            break;
        }
    }
}

/**
 * server_update_bullets - Update all bullets
 */
static void server_update_bullets(GameServer* server, float dt) {
    for (int i = 0; i < MAX_SERVER_BULLETS; i++) {
        ServerBullet* bullet = &server->bullets[i];
        if (!bullet->active) continue;

        // Update position
        bullet->x += bullet->vx * dt;
        bullet->y += bullet->vy * dt;

        // Update lifetime
        bullet->lifetime -= dt;

        // Remove if expired or out of bounds
        if (bullet->lifetime <= 0 ||
            bullet->x < 0 || bullet->x > GAME_WIDTH ||
            bullet->y < 0 || bullet->y > GAME_HEIGHT) {
            bullet->active = 0;
            server->bullet_count--;
        }
    }
}

/**
 * get_weapon_cooldown - Get fire cooldown based on weapon type
 */
static float get_weapon_cooldown(uint8_t weapon_type) {
    switch (weapon_type) {
        case WEAPON_TYPE_SPREAD: return 1.0f / SPREAD_FIRE_RATE;
        case WEAPON_TYPE_RAPID:  return 1.0f / RAPID_FIRE_RATE;
        case WEAPON_TYPE_LASER:  return 1.0f / LASER_FIRE_RATE;
        default:                 return 1.0f / SPREAD_FIRE_RATE;
    }
}

/**
 * server_handle_firing - Process fire input from players
 */
static void server_handle_firing(GameServer* server, float dt) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        ServerPlayer* player = &server->players[i];
        if (!player->active) continue;

        // Update cooldown
        if (player->fire_cooldown > 0) {
            player->fire_cooldown -= dt;
        }

        // Check if firing
        if ((player->input_flags & INPUT_FIRE) && player->fire_cooldown <= 0) {
            server_spawn_bullet(server, i, player->x, player->y);
            player->fire_cooldown = get_weapon_cooldown(player->weapon);
        }
    }
}

/**
 * server_update_physics - Update game physics based on player input
 *
 * CONCEPT: Server-Side Physics
 * ============================
 * The server runs the same physics that the client would.
 * Clients send input, server calculates results.
 *
 * This prevents cheating - clients can't lie about their position.
 *
 * IMPORTANT: Uses shared constants from protocol.h to match client physics!
 */
static void server_update_physics(GameServer* server, float dt) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        ServerPlayer* player = &server->players[i];
        if (!player->active) continue;

        // Apply input to velocity (same formula as client)
        float accel_x = 0, accel_y = 0;
        if (player->input_flags & INPUT_UP)    accel_y = -1.0f;
        if (player->input_flags & INPUT_DOWN)  accel_y = 1.0f;
        if (player->input_flags & INPUT_LEFT)  accel_x = -1.0f;
        if (player->input_flags & INPUT_RIGHT) accel_x = 1.0f;

        // Normalize diagonal movement
        if (accel_x != 0 && accel_y != 0) {
            float inv_sqrt2 = 0.7071f;
            accel_x *= inv_sqrt2;
            accel_y *= inv_sqrt2;
        }

        // Apply acceleration
        player->vx += accel_x * PLAYER_ACCELERATION * dt;
        player->vy += accel_y * PLAYER_ACCELERATION * dt;

        // Apply friction (adjusted for frame rate)
        float friction = powf(PLAYER_FRICTION, dt * 60.0f);
        player->vx *= friction;
        player->vy *= friction;

        // Clamp velocity to max speed
        float speed = sqrtf(player->vx * player->vx + player->vy * player->vy);
        if (speed > PLAYER_SPEED) {
            float scale = PLAYER_SPEED / speed;
            player->vx *= scale;
            player->vy *= scale;
        }

        // Stop if very slow
        if (fabsf(player->vx) < 1.0f) player->vx = 0;
        if (fabsf(player->vy) < 1.0f) player->vy = 0;

        // Update position
        player->x += player->vx * dt;
        player->y += player->vy * dt;

        // Clamp to screen bounds (using shared constants)
        float hw = 32, hh = 32;  // Half player size
        if (player->x < hw) { player->x = hw; player->vx = 0; }
        if (player->x > GAME_WIDTH - hw) { player->x = GAME_WIDTH - hw; player->vx = 0; }
        if (player->y < hh) { player->y = hh; player->vy = 0; }
        if (player->y > GAME_HEIGHT - hh) { player->y = GAME_HEIGHT - hh; player->vy = 0; }
    }
}

/**
 * server_send_state - Send game state to all clients
 */
static void server_send_state(GameServer* server) {
    // Count active bullets (capped at MAX_SYNC_BULLETS)
    int bullet_count = 0;
    for (int i = 0; i < MAX_SERVER_BULLETS && bullet_count < MAX_SYNC_BULLETS; i++) {
        if (server->bullets[i].active) {
            bullet_count++;
        }
    }

    // Build game state message
    // Size = header + GameStateMsg base + (player_count * PlayerState) + (bullet_count * BulletState)
    int state_size = sizeof(GameStateMsg) +
                     server->player_count * sizeof(PlayerState) +
                     bullet_count * sizeof(BulletState);
    int total_size = sizeof(MessageHeader) + state_size;

    uint8_t* buffer = malloc(total_size);
    if (buffer == NULL) return;

    // Fill header
    MessageHeader* header = (MessageHeader*)buffer;
    header->type = MSG_GAME_STATE;
    header->length = state_size;

    // Fill game state
    GameStateMsg* state = (GameStateMsg*)(buffer + sizeof(MessageHeader));
    state->tick = server->tick;
    state->your_sequence = 0;  // Will be overwritten per-player
    state->player_count = server->player_count;
    state->bullet_count = (uint8_t)bullet_count;

    // Fill player states
    int idx = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        ServerPlayer* sp = &server->players[i];
        if (!sp->active) continue;

        PlayerState* ps = &state->players[idx];
        ps->player_id = (uint8_t)i;
        ps->x = sp->x;
        ps->y = sp->y;
        ps->vx = sp->vx;
        ps->vy = sp->vy;
        ps->health = (int16_t)sp->health;
        ps->weapon = sp->weapon;
        ps->flags = (sp->input_flags & INPUT_FIRE) ? 1 : 0;  // Flag if firing
        idx++;
    }

    // Fill bullet states (after player states)
    BulletState* bullets_data = (BulletState*)(buffer + sizeof(MessageHeader) +
                                                sizeof(GameStateMsg) +
                                                server->player_count * sizeof(PlayerState));
    int bidx = 0;
    for (int i = 0; i < MAX_SERVER_BULLETS && bidx < bullet_count; i++) {
        ServerBullet* sb = &server->bullets[i];
        if (!sb->active) continue;

        BulletState* bs = &bullets_data[bidx];
        bs->owner_id = sb->owner_id;
        bs->x = sb->x;
        bs->y = sb->y;
        bs->vx = sb->vx;
        bs->vy = sb->vy;
        bs->weapon_type = sb->weapon_type;
        bidx++;
    }

    // Send to each client
    for (int i = 0; i < MAX_PLAYERS; i++) {
        ServerPlayer* player = &server->players[i];
        if (!player->active) continue;

        // Customize the sequence number for this player
        state->your_sequence = player->last_sequence;

        // Send the state - if it fails, disconnect the player
        if (net_send_all(player->socket, buffer, total_size) < 0) {
            printf("Failed to send state to player %d, disconnecting\n", i);
            net_close(player->socket);
            player->active = 0;
            server->player_count--;
        }
    }

    free(buffer);
}

/**
 * main - Server entry point
 */
int main(int argc, char* argv[]) {
    uint16_t port = SERVER_PORT;

    // Parse command line arguments
    if (argc > 1) {
        port = (uint16_t)atoi(argv[1]);
    }

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║     VOID DRIFTER SERVER - Module 4: Networking             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    // Set up signal handler for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Ignore SIGPIPE - we handle send errors gracefully instead of crashing
    // SIGPIPE is sent when writing to a socket that the other end has closed
    signal(SIGPIPE, SIG_IGN);

    // Initialize networking
    if (net_init() != 0) {
        fprintf(stderr, "Failed to initialize networking\n");
        return 1;
    }

    // Initialize server
    GameServer server;
    if (server_init(&server, port) != 0) {
        net_cleanup();
        return 1;
    }

    // Make listen socket non-blocking so we can check for new connections
    // without blocking the main loop
    net_set_nonblocking(server.listen_socket);

    printf("Server running. Press Ctrl+C to stop.\n\n");

    // Main server loop
    // In a real game, this would run at TICK_RATE ticks per second.
    // For simplicity, we just use usleep().

    float dt = 1.0f / TICK_RATE;

    while (g_running) {
        // Check for new connections
        server_accept_new_client(&server);

        // Process messages from each connected client
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (server.players[i].active) {
                // Make client socket non-blocking for polling
                net_set_nonblocking(server.players[i].socket);
                server_handle_client_message(&server, i);
            }
        }

        // Update game physics
        server_update_physics(&server, dt);

        // Handle firing and update bullets
        server_handle_firing(&server, dt);
        server_update_bullets(&server, dt);

        // Send state to all clients
        if (server.player_count > 0) {
            server_send_state(&server);
        }

        // Increment tick
        server.tick++;

        // Sleep to maintain tick rate
        usleep((useconds_t)(dt * 1000000));
    }

    // Cleanup
    server_cleanup(&server);
    net_cleanup();

    printf("Server stopped.\n");
    return 0;
}
