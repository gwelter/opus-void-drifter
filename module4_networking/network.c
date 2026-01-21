/**
 * network.c - Socket Networking Implementation
 *
 * This implements the POSIX socket API wrappers.
 *
 * DEEP DIVE: The Socket System Calls
 * ==================================
 *
 * socket()  - Create a new socket (returns file descriptor)
 * bind()    - Associate socket with local address/port
 * listen()  - Mark socket as accepting connections
 * accept()  - Wait for and accept a client (returns NEW socket)
 * connect() - Connect to a remote server
 * send()    - Send data on connected socket
 * recv()    - Receive data from connected socket
 * close()   - Close a socket
 *
 * Each of these is a SYSTEM CALL - a request to the OS kernel.
 */

#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // For close()
#include <fcntl.h>       // For fcntl() (non-blocking mode)
#include <errno.h>       // For errno
#include <netdb.h>       // For gethostbyname()

/**
 * net_init - Initialize networking
 *
 * Unix doesn't need initialization.
 * Windows would call WSAStartup() here.
 */
int net_init(void) {
    // No-op on Unix
    return 0;
}

/**
 * net_cleanup - Cleanup networking
 */
void net_cleanup(void) {
    // No-op on Unix
}

/**
 * net_create_server - Create a listening server socket
 *
 * STEP BY STEP:
 * 1. socket()  - Create the socket
 * 2. setsockopt() - Allow address reuse (avoids "address in use" errors)
 * 3. bind()    - Associate with port
 * 4. listen()  - Start accepting connections
 */
Socket net_create_server(uint16_t port, int backlog) {
    // --- STEP 1: Create socket ---
    //
    // AF_INET     = IPv4
    // SOCK_STREAM = TCP (reliable, ordered, connection-based)
    // 0           = Let kernel choose protocol (TCP for SOCK_STREAM)
    //
    // For UDP, use SOCK_DGRAM instead.

    Socket server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        perror("socket() failed");
        return INVALID_SOCKET;
    }

    // --- STEP 2: Set socket options ---
    //
    // SO_REUSEADDR allows binding to a port that was recently used.
    // Without this, you'd get "Address already in use" for ~2 minutes
    // after stopping a server.

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(server_fd);
        return INVALID_SOCKET;
    }

    // --- STEP 3: Bind to address ---
    //
    // struct sockaddr_in describes an IPv4 address:
    //     sin_family = AF_INET (IPv4)
    //     sin_addr   = IP address (INADDR_ANY = any local interface)
    //     sin_port   = Port number (in NETWORK byte order!)

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));  // Zero out the struct
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
    address.sin_port = htons(port);        // htons = Host TO Network Short

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind() failed");
        close(server_fd);
        return INVALID_SOCKET;
    }

    // --- STEP 4: Start listening ---
    //
    // The backlog parameter is how many pending connections to queue.
    // If the queue is full, new connections are refused.

    if (listen(server_fd, backlog) < 0) {
        perror("listen() failed");
        close(server_fd);
        return INVALID_SOCKET;
    }

    return server_fd;
}

/**
 * net_accept_client - Accept a waiting client
 *
 * BLOCKING BEHAVIOR:
 * ==================
 * By default, accept() blocks until a client connects.
 * This is why we need threads in Module 5 - we can't block the game loop!
 */
Socket net_accept_client(Socket server_socket, struct sockaddr_in* client_addr) {
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // accept() creates a NEW socket for this specific client.
    // The original server_socket continues listening for more clients.
    Socket client_fd = accept(server_socket, (struct sockaddr*)client_addr, &addr_len);

    if (client_fd == INVALID_SOCKET) {
        // EAGAIN/EWOULDBLOCK means no client waiting (in non-blocking mode)
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept() failed");
        }
        return INVALID_SOCKET;
    }

    return client_fd;
}

/**
 * net_connect_to_server - Connect as a client
 *
 * STEP BY STEP:
 * 1. Resolve hostname to IP address (if needed)
 * 2. Create socket
 * 3. Connect to server
 */
Socket net_connect_to_server(const char* host, uint16_t port) {
    // --- STEP 1: Resolve hostname ---
    //
    // If 'host' is an IP like "127.0.0.1", inet_addr() converts it directly.
    // If it's a hostname like "localhost", we need DNS resolution.

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Try to parse as IP address first
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        // Not a valid IP, try DNS resolution
        struct hostent* he = gethostbyname(host);
        if (he == NULL) {
            fprintf(stderr, "Could not resolve hostname: %s\n", host);
            return INVALID_SOCKET;
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    // --- STEP 2: Create socket ---
    Socket sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        perror("socket() failed");
        return INVALID_SOCKET;
    }

    // --- STEP 3: Connect ---
    //
    // connect() blocks until the connection is established or fails.
    // Timeout is typically ~30 seconds for TCP.

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        close(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

/**
 * net_send_all - Send all data, handling partial sends
 *
 * IMPORTANT: TCP doesn't guarantee all data is sent at once!
 *
 * Example of partial send:
 *     bytes_to_send = 1000
 *     first send():  sent 400 bytes
 *     second send(): sent 400 bytes
 *     third send():  sent 200 bytes
 *     total:         1000 bytes
 */
int net_send_all(Socket socket, const void* data, int length) {
    const char* ptr = (const char*)data;
    int total_sent = 0;

    while (total_sent < length) {
        int bytes_sent = send(socket, ptr + total_sent, length - total_sent, 0);

        if (bytes_sent < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, try again
                continue;
            }
            perror("send() failed");
            return -1;
        }

        if (bytes_sent == 0) {
            // Connection closed
            return total_sent;
        }

        total_sent += bytes_sent;
    }

    return total_sent;
}

/**
 * net_recv_all - Receive exactly N bytes
 *
 * Like send(), recv() might return partial data.
 * This function loops until we have exactly 'length' bytes.
 */
int net_recv_all(Socket socket, void* buffer, int length) {
    char* ptr = (char*)buffer;
    int total_received = 0;

    while (total_received < length) {
        int bytes_received = recv(socket, ptr + total_received,
                                   length - total_received, 0);

        if (bytes_received < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, try again
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available (non-blocking mode)
                // Return what we have so far
                return total_received;
            }
            perror("recv() failed");
            return -1;
        }

        if (bytes_received == 0) {
            // Connection closed by peer
            return total_received;
        }

        total_received += bytes_received;
    }

    return total_received;
}

/**
 * net_close - Close a socket
 */
void net_close(Socket socket) {
    if (socket != INVALID_SOCKET) {
        close(socket);
    }
}

/**
 * net_set_nonblocking - Make socket non-blocking
 *
 * Uses fcntl() to add the O_NONBLOCK flag.
 */
int net_set_nonblocking(Socket socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        return -1;
    }

    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        return -1;
    }

    return 0;
}

/**
 * net_get_error_string - Get error description
 */
const char* net_get_error_string(void) {
    return strerror(errno);
}

/**
 * net_addr_to_string - Convert address to string
 *
 * Formats as "xxx.xxx.xxx.xxx:port"
 */
char* net_addr_to_string(const struct sockaddr_in* addr, char* buffer, int size) {
    if (addr == NULL || buffer == NULL || size < 22) {
        return NULL;
    }

    // inet_ntoa converts IP to string
    // ntohs converts port from network to host byte order
    snprintf(buffer, size, "%s:%d",
             inet_ntoa(addr->sin_addr),
             ntohs(addr->sin_port));

    return buffer;
}
