/**
 * network_client.h - Network Client Thread
 *
 * CONCEPT: Threaded Network Client
 * ================================
 * This module spawns a dedicated thread for network operations.
 * The thread can block on recv() without affecting the game loop.
 *
 * Communication with main thread:
 *     - SharedState: Mutex-protected data exchange
 *     - running flag: Signals thread to stop
 */

#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <pthread.h>
#include <stdint.h>
#include "shared_state.h"

// Forward declaration
typedef struct NetworkClient NetworkClient;

/**
 * NetworkClient - Client state and thread handle
 */
struct NetworkClient {
    // Thread
    pthread_t thread;
    volatile int running;       // Thread checks this to know when to stop

    // Connection
    int socket;
    char host[64];
    uint16_t port;

    // Reference to shared state (owned by main, not us)
    SharedState* shared;

    // Our player ID (assigned by server)
    uint8_t player_id;
};

/**
 * net_client_create - Create and initialize a network client
 *
 * @return  Pointer to new client, or NULL on failure
 */
NetworkClient* net_client_create(void);

/**
 * net_client_destroy - Destroy a network client
 *
 * Stops the thread if running and frees resources.
 *
 * @param client  Client to destroy
 */
void net_client_destroy(NetworkClient* client);

/**
 * net_client_connect - Start connection to server
 *
 * This spawns the network thread which will:
 *     1. Connect to the server
 *     2. Handle send/receive in a loop
 *     3. Update SharedState with received data
 *
 * @param client  The client
 * @param shared  Shared state (must outlive the client!)
 * @param host    Server hostname/IP
 * @param port    Server port
 * @return        0 on success, -1 on failure
 */
int net_client_connect(NetworkClient* client, SharedState* shared,
                       const char* host, uint16_t port);

/**
 * net_client_disconnect - Stop the network thread
 *
 * Signals the thread to stop and waits for it to finish.
 *
 * @param client  The client
 */
void net_client_disconnect(NetworkClient* client);

/**
 * net_client_is_connected - Check if connected
 *
 * @param client  The client
 * @return        1 if connected, 0 otherwise
 */
int net_client_is_connected(NetworkClient* client);

#endif // NETWORK_CLIENT_H
