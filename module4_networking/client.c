/**
 * client.c - Void Drifter Network Client
 *
 * This is a simple client that:
 *     1. Connects to the game server
 *     2. Sends player input
 *     3. Receives and displays game state
 *
 * This is a CLI client for educational purposes.
 * In Module 5, we'll integrate networking with the Raylib game.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>  // For non-blocking keyboard input

#include "protocol.h"
#include "network.h"

// Global running flag
static volatile int g_running = 1;

/**
 * ClientState - Client-side game state
 */
typedef struct {
    Socket socket;          // Connection to server
    uint8_t player_id;      // Our assigned player ID
    uint32_t sequence;      // Input sequence number

    // Local view of game state (received from server)
    PlayerState players[MAX_CLIENTS];
    int player_count;
    uint32_t last_tick;     // Last server tick received

    // Our input state
    uint8_t input_flags;
} ClientState;

/**
 * signal_handler - Handle Ctrl+C
 */
static void signal_handler(int sig) {
    (void)sig;
    printf("\nDisconnecting...\n");
    g_running = 0;
}

/**
 * Terminal state for non-blocking input
 */
static struct termios g_old_termios;
static int g_terminal_configured = 0;

/**
 * setup_terminal - Configure terminal for non-blocking input
 *
 * CONCEPT: Terminal Raw Mode
 * ==========================
 * By default, terminal input is "cooked" - the OS buffers input until Enter.
 * We need "raw" mode to read keypresses immediately.
 *
 * This is similar to how game consoles handle input.
 */
static void setup_terminal(void) {
    struct termios new_termios;

    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &g_old_termios);
    new_termios = g_old_termios;

    // Disable canonical mode (line buffering) and echo
    new_termios.c_lflag &= ~(ICANON | ECHO);

    // Set minimum characters and timeout for read
    new_termios.c_cc[VMIN] = 0;   // Don't block
    new_termios.c_cc[VTIME] = 0;  // No timeout

    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    g_terminal_configured = 1;
}

/**
 * restore_terminal - Restore normal terminal settings
 */
static void restore_terminal(void) {
    if (g_terminal_configured) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
        g_terminal_configured = 0;
    }
}

/**
 * client_connect - Connect to the game server
 */
static int client_connect(ClientState* client, const char* host, uint16_t port) {
    printf("Connecting to %s:%d...\n", host, port);

    client->socket = net_connect_to_server(host, port);
    if (client->socket == INVALID_SOCKET) {
        fprintf(stderr, "Failed to connect to server\n");
        return -1;
    }

    printf("Connected! Waiting for server response...\n");

    // Wait for connection acknowledgement
    MessageHeader header;
    if (net_recv_all(client->socket, &header, sizeof(header)) <= 0) {
        fprintf(stderr, "Failed to receive connection response\n");
        net_close(client->socket);
        return -1;
    }

    if (header.type != MSG_CONNECT_ACK) {
        fprintf(stderr, "Unexpected message type: %d\n", header.type);
        net_close(client->socket);
        return -1;
    }

    ConnectAckMsg ack;
    if (net_recv_all(client->socket, &ack, sizeof(ack)) <= 0) {
        fprintf(stderr, "Failed to receive ack data\n");
        net_close(client->socket);
        return -1;
    }

    if (!ack.success) {
        fprintf(stderr, "Connection rejected (reason: %d)\n", ack.reason);
        net_close(client->socket);
        return -1;
    }

    client->player_id = ack.player_id;
    client->sequence = 0;

    printf("Joined as Player %d!\n\n", client->player_id);
    return 0;
}

/**
 * client_disconnect - Disconnect from server
 */
static void client_disconnect(ClientState* client) {
    // Send disconnect message
    MessageHeader header = { .type = MSG_DISCONNECT, .length = 0 };
    net_send_all(client->socket, &header, sizeof(header));

    net_close(client->socket);
    client->socket = INVALID_SOCKET;
}

/**
 * client_send_input - Send current input to server
 */
static void client_send_input(ClientState* client) {
    client->sequence++;

    PlayerInputMsg input = {
        .player_id = client->player_id,
        .input_flags = client->input_flags,
        .sequence = client->sequence
    };

    MessageHeader header = {
        .type = MSG_PLAYER_INPUT,
        .length = sizeof(input)
    };

    net_send_all(client->socket, &header, sizeof(header));
    net_send_all(client->socket, &input, sizeof(input));
}

/**
 * client_receive_state - Receive game state from server
 */
static int client_receive_state(ClientState* client) {
    // Check if data is available (non-blocking)
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client->socket, &read_fds);

    struct timeval timeout = { 0, 0 };  // No wait
    if (select(client->socket + 1, &read_fds, NULL, NULL, &timeout) <= 0) {
        return 0;  // No data available
    }

    // Read header
    MessageHeader header;
    int bytes = net_recv_all(client->socket, &header, sizeof(header));
    if (bytes <= 0) {
        printf("Server disconnected\n");
        return -1;
    }

    if (header.type == MSG_GAME_STATE) {
        // Read game state header (tick, your_sequence, player_count)
        // The sizeof(GameStateMsg) is just the fixed fields since players[] is flexible
        GameStateMsg state_header;
        if (net_recv_all(client->socket, &state_header, sizeof(GameStateMsg)) <= 0) {
            return -1;
        }

        client->last_tick = state_header.tick;
        client->player_count = state_header.player_count;

        // Read player states
        for (int i = 0; i < state_header.player_count && i < MAX_CLIENTS; i++) {
            if (net_recv_all(client->socket, &client->players[i],
                            sizeof(PlayerState)) <= 0) {
                return -1;
            }
        }

        return 1;  // Received state
    }

    return 0;  // Other message type
}

/**
 * client_handle_input - Check keyboard and update input flags
 */
static void client_handle_input(ClientState* client) {
    // Read available keypresses
    char c;
    uint8_t new_flags = 0;

    // Clear previous flags
    client->input_flags = 0;

    // Read all available keypresses
    while (read(STDIN_FILENO, &c, 1) > 0) {
        switch (c) {
            case 'w': case 'W': new_flags |= INPUT_UP; break;
            case 's': case 'S': new_flags |= INPUT_DOWN; break;
            case 'a': case 'A': new_flags |= INPUT_LEFT; break;
            case 'd': case 'D': new_flags |= INPUT_RIGHT; break;
            case ' ': new_flags |= INPUT_FIRE; break;
            case 'q': case 'Q': g_running = 0; break;
            case 27: g_running = 0; break;  // ESC
        }
    }

    client->input_flags = new_flags;
}

/**
 * client_print_state - Display current game state
 */
static void client_print_state(const ClientState* client) {
    // Clear screen (ANSI escape code)
    printf("\033[2J\033[H");

    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║     VOID DRIFTER CLIENT - Module 4                        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    printf("Server Tick: %u    Your ID: %d\n\n", client->last_tick, client->player_id);

    printf("Players (%d connected):\n", client->player_count);
    printf("┌────────┬────────────────────┬─────────────────┬────────┐\n");
    printf("│   ID   │     Position       │    Velocity     │ Health │\n");
    printf("├────────┼────────────────────┼─────────────────┼────────┤\n");

    for (int i = 0; i < client->player_count; i++) {
        const PlayerState* p = &client->players[i];
        char marker = (p->player_id == client->player_id) ? '*' : ' ';
        printf("│  %c%d    │ (%6.1f, %6.1f)   │ (%5.1f, %5.1f)  │  %3d   │\n",
               marker, p->player_id, p->x, p->y, p->vx, p->vy, p->health);
    }
    printf("└────────┴────────────────────┴─────────────────┴────────┘\n");
    printf("\n* = You\n\n");

    printf("Your Input: ");
    if (client->input_flags & INPUT_UP)    printf("[UP] ");
    if (client->input_flags & INPUT_DOWN)  printf("[DOWN] ");
    if (client->input_flags & INPUT_LEFT)  printf("[LEFT] ");
    if (client->input_flags & INPUT_RIGHT) printf("[RIGHT] ");
    if (client->input_flags & INPUT_FIRE)  printf("[FIRE] ");
    if (client->input_flags == 0) printf("(none)");
    printf("\n\n");

    printf("Controls: WASD to move, SPACE to fire, Q to quit\n");
}

/**
 * main - Client entry point
 */
int main(int argc, char* argv[]) {
    const char* host = "127.0.0.1";
    uint16_t port = DEFAULT_PORT;

    // Parse command line arguments
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = (uint16_t)atoi(argv[2]);
    }

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║     VOID DRIFTER CLIENT - Module 4: Networking             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize networking
    if (net_init() != 0) {
        fprintf(stderr, "Failed to initialize networking\n");
        return 1;
    }

    // Set up terminal for non-blocking input
    setup_terminal();

    // Initialize client state
    ClientState client;
    memset(&client, 0, sizeof(client));
    client.socket = INVALID_SOCKET;

    // Connect to server
    if (client_connect(&client, host, port) != 0) {
        restore_terminal();
        net_cleanup();
        return 1;
    }

    // Make socket non-blocking for receive
    net_set_nonblocking(client.socket);

    // Main client loop
    while (g_running) {
        // Handle keyboard input
        client_handle_input(&client);

        // Send input to server (if we have any)
        client_send_input(&client);

        // Receive game state from server
        int result = client_receive_state(&client);
        if (result < 0) {
            g_running = 0;
            break;
        }

        // Display current state
        client_print_state(&client);

        // Sleep briefly
        usleep(16667);  // ~60 FPS
    }

    // Disconnect
    client_disconnect(&client);
    restore_terminal();
    net_cleanup();

    printf("Disconnected from server.\n");
    return 0;
}
