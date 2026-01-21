/**
 * network_client.c - Network Client Thread Implementation
 *
 * This file implements the network thread that:
 *     1. Connects to the game server
 *     2. Sends player input
 *     3. Receives game state updates
 *     4. Updates SharedState for the main thread
 *
 * CONCEPT: Thread Function Pattern
 * ================================
 * The thread function signature is: void* func(void* arg)
 *     - Takes a single void* argument (can be cast to anything)
 *     - Returns void* (usually NULL, or an exit code)
 *     - Runs until it returns or is cancelled
 */

#include "network_client.h"
#include "network.h"
#include "protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

// How often to send input (in microseconds)
#define SEND_INTERVAL_US 16667  // ~60Hz

/**
 * thread_send_input - Send current input to server
 */
static void thread_send_input(NetworkClient* client) {
    uint32_t sequence;
    uint8_t weapon_type;
    uint8_t flags = shared_state_get_input(client->shared, &sequence, &weapon_type);

    PlayerInputMsg input = {
        .player_id = client->player_id,
        .input_flags = flags,
        .weapon_type = weapon_type,
        .sequence = sequence
    };

    MessageHeader header = {
        .type = MSG_PLAYER_INPUT,
        .length = sizeof(input)
    };

    net_send_all(client->socket, &header, sizeof(header));
    net_send_all(client->socket, &input, sizeof(input));

    // Update stats
    shared_state_lock(client->shared);
    client->shared->packets_sent++;
    shared_state_unlock(client->shared);
}

/**
 * network_thread_func - The main thread function
 *
 * LIFECYCLE:
 * 1. Connect to server
 * 2. Wait for connection acknowledgement (BLOCKING - reliable handshake)
 * 3. Switch to non-blocking mode
 * 4. Loop: Send input, receive state
 * 5. Disconnect when running == false
 */
static void* network_thread_func(void* arg) {
    NetworkClient* client = (NetworkClient*)arg;

    printf("DEBUG: Network thread starting, connecting to %s:%d\n",
           client->host, client->port);

    // Connect to server
    shared_state_set_status(client->shared, NET_CONNECTING, "Connecting...");

    client->socket = net_connect_to_server(client->host, client->port);
    if (client->socket < 0) {
        printf("DEBUG: Failed to connect to server\n");
        shared_state_set_status(client->shared, NET_ERROR, "Failed to connect");
        client->running = 0;
        return NULL;
    }
    printf("DEBUG: TCP connection established, socket=%d\n", client->socket);

    // Send connect request
    ConnectMsg connect_msg = {
        .version = PROTOCOL_VERSION
    };
    strncpy(connect_msg.name, "Player", sizeof(connect_msg.name));

    MessageHeader header = {
        .type = MSG_CONNECT,
        .length = sizeof(connect_msg)
    };

    printf("DEBUG: Sending MSG_CONNECT (header=%lu bytes, payload=%lu bytes)\n",
           sizeof(header), sizeof(connect_msg));

    int sent1 = net_send_all(client->socket, &header, sizeof(header));
    int sent2 = net_send_all(client->socket, &connect_msg, sizeof(connect_msg));
    printf("DEBUG: Sent header=%d, payload=%d\n", sent1, sent2);

    if (sent1 < 0 || sent2 < 0) {
        printf("DEBUG: Failed to send connect message\n");
        shared_state_set_status(client->shared, NET_ERROR, "Failed to send connect");
        client->running = 0;
        net_close(client->socket);
        client->socket = -1;
        return NULL;
    }

    // --- BLOCKING HANDSHAKE ---
    // Wait for MSG_CONNECT_ACK before going non-blocking
    // This ensures reliable connection establishment
    printf("DEBUG: Waiting for MSG_CONNECT_ACK...\n");
    MessageHeader ack_header;
    int recv_bytes = net_recv_all(client->socket, &ack_header, sizeof(ack_header));
    printf("DEBUG: Received ack_header: %d bytes (expected %lu), type=%d\n",
           recv_bytes, sizeof(ack_header), ack_header.type);

    if (recv_bytes != sizeof(ack_header)) {
        printf("DEBUG: Failed to receive ack header\n");
        shared_state_set_status(client->shared, NET_ERROR, "No response from server");
        client->running = 0;
        net_close(client->socket);
        client->socket = -1;
        return NULL;
    }

    if (ack_header.type != MSG_CONNECT_ACK) {
        printf("DEBUG: Unexpected message type: %d (expected %d)\n",
               ack_header.type, MSG_CONNECT_ACK);
        shared_state_set_status(client->shared, NET_ERROR, "Unexpected response");
        client->running = 0;
        net_close(client->socket);
        client->socket = -1;
        return NULL;
    }

    ConnectAckMsg ack;
    recv_bytes = net_recv_all(client->socket, &ack, sizeof(ack));
    printf("DEBUG: Received ack payload: %d bytes (expected %lu), success=%d, player_id=%d\n",
           recv_bytes, sizeof(ack), ack.success, ack.player_id);

    if (recv_bytes != sizeof(ack)) {
        printf("DEBUG: Failed to receive ack payload\n");
        shared_state_set_status(client->shared, NET_ERROR, "Incomplete ACK");
        client->running = 0;
        net_close(client->socket);
        client->socket = -1;
        return NULL;
    }

    if (!ack.success) {
        const char* reason = (ack.reason == 0) ? "Server full" : "Version mismatch";
        printf("DEBUG: Connection rejected: %s\n", reason);
        shared_state_set_status(client->shared, NET_ERROR, reason);
        client->running = 0;
        net_close(client->socket);
        client->socket = -1;
        return NULL;
    }

    // Successfully connected!
    client->player_id = ack.player_id;
    shared_state_lock(client->shared);
    client->shared->my_id = ack.player_id;
    shared_state_unlock(client->shared);
    shared_state_set_status(client->shared, NET_CONNECTED, "Connected!");
    printf("DEBUG: Successfully connected as player %d\n", client->player_id);

    // NOW make socket non-blocking for the game loop
    net_set_nonblocking(client->socket);

    // Main loop
    while (client->running) {
        // --- RECEIVE ---
        // Try to read without blocking
        MessageHeader msg_header;
        int bytes = recv(client->socket, &msg_header, sizeof(msg_header), 0);

        if (bytes == sizeof(msg_header)) {
            // Got a complete header, read the payload
            uint8_t payload[BUFFER_SIZE];

            switch (msg_header.type) {
                case MSG_GAME_STATE: {
                    // Read the fixed part of GameStateMsg
                    GameStateMsg state_hdr;
                    int state_bytes = recv(client->socket, &state_hdr, sizeof(GameStateMsg), 0);
                    if (state_bytes != sizeof(GameStateMsg)) {
                        // Partial or no data - skip this frame
                        if (state_bytes > 0) {
                            printf("DEBUG: Partial GameStateMsg: got %d, expected %lu\n",
                                   state_bytes, sizeof(GameStateMsg));
                        }
                        break;
                    }
                    if (state_bytes == sizeof(GameStateMsg)) {
                        RemotePlayer players[MAX_PLAYERS];
                        int player_count = (state_hdr.player_count > MAX_PLAYERS)
                                           ? MAX_PLAYERS : state_hdr.player_count;

                        int all_received = 1;

                        // Receive player states
                        for (int i = 0; i < player_count; i++) {
                            PlayerState ps;
                            int ps_bytes = recv(client->socket, &ps, sizeof(ps), 0);
                            if (ps_bytes == sizeof(ps)) {
                                players[i].active = 1;
                                players[i].id = ps.player_id;
                                players[i].x = ps.x;
                                players[i].y = ps.y;
                                players[i].vx = ps.vx;
                                players[i].vy = ps.vy;
                                players[i].health = ps.health;
                                players[i].weapon = ps.weapon;
                            } else {
                                all_received = 0;
                                break;
                            }
                        }

                        // Receive bullet states
                        RemoteBullet bullets[MAX_REMOTE_BULLETS];
                        int bullet_count = (state_hdr.bullet_count > MAX_REMOTE_BULLETS)
                                           ? MAX_REMOTE_BULLETS : state_hdr.bullet_count;

                        if (all_received) {
                            for (int i = 0; i < bullet_count; i++) {
                                BulletState bs;
                                int bs_bytes = recv(client->socket, &bs, sizeof(bs), 0);
                                if (bs_bytes == sizeof(bs)) {
                                    bullets[i].active = 1;
                                    bullets[i].owner_id = bs.owner_id;
                                    bullets[i].x = bs.x;
                                    bullets[i].y = bs.y;
                                    bullets[i].vx = bs.vx;
                                    bullets[i].vy = bs.vy;
                                    bullets[i].weapon_type = bs.weapon_type;
                                } else {
                                    all_received = 0;
                                    break;
                                }
                            }
                        }

                        if (all_received) {
                            shared_state_update_players(client->shared, players, player_count,
                                                        state_hdr.tick);
                            shared_state_update_bullets(client->shared, bullets, bullet_count);
                        }
                    }
                    break;
                }

                default:
                    // Skip unknown message
                    if (msg_header.length > 0 && msg_header.length < BUFFER_SIZE) {
                        recv(client->socket, payload, msg_header.length, 0);
                    }
                    break;
            }
        } else if (bytes == 0) {
            // Connection closed
            printf("DEBUG: Server closed connection (recv returned 0)\n");
            shared_state_set_status(client->shared, NET_DISCONNECTED, "Server closed");
            client->running = 0;
            break;
        } else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            // Actual error
            printf("DEBUG: recv error: %s (errno=%d)\n", strerror(errno), errno);
            shared_state_set_status(client->shared, NET_ERROR, "Connection error");
            client->running = 0;
            break;
        }
        // bytes < 0 with EAGAIN/EWOULDBLOCK means no data available - that's fine

        // --- SEND ---
        thread_send_input(client);

        // Sleep briefly to avoid spinning
        usleep(SEND_INTERVAL_US);
    }

    // Cleanup
    printf("Network thread exiting\n");
    if (client->socket >= 0) {
        net_close(client->socket);
        client->socket = -1;
    }

    return NULL;
}

/**
 * net_client_create - Create a network client
 */
NetworkClient* net_client_create(void) {
    NetworkClient* client = malloc(sizeof(NetworkClient));
    if (client == NULL) return NULL;

    memset(client, 0, sizeof(NetworkClient));
    client->socket = -1;
    client->running = 0;

    return client;
}

/**
 * net_client_destroy - Destroy a network client
 */
void net_client_destroy(NetworkClient* client) {
    if (client == NULL) return;

    // Stop thread if running
    net_client_disconnect(client);

    free(client);
}

/**
 * net_client_connect - Start connection (spawns thread)
 */
int net_client_connect(NetworkClient* client, SharedState* shared,
                       const char* host, uint16_t port) {
    if (client == NULL || shared == NULL || host == NULL) return -1;

    // Store connection info
    strncpy(client->host, host, sizeof(client->host) - 1);
    client->port = port;
    client->shared = shared;
    client->running = 1;

    // Create the network thread
    if (pthread_create(&client->thread, NULL, network_thread_func, client) != 0) {
        fprintf(stderr, "Failed to create network thread\n");
        client->running = 0;
        return -1;
    }

    return 0;
}

/**
 * net_client_disconnect - Stop the network thread
 */
void net_client_disconnect(NetworkClient* client) {
    if (client == NULL) return;

    if (client->running) {
        // Signal thread to stop
        client->running = 0;

        // Wait for thread to finish
        pthread_join(client->thread, NULL);
    }

    // Close socket if still open
    if (client->socket >= 0) {
        net_close(client->socket);
        client->socket = -1;
    }
}

/**
 * net_client_is_connected - Check connection status
 */
int net_client_is_connected(NetworkClient* client) {
    if (client == NULL || client->shared == NULL) return 0;
    return shared_state_get_status(client->shared) == NET_CONNECTED;
}
