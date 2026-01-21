/**
 * network.h - Socket Networking Utilities
 *
 * CONCEPT: Abstraction Layer
 * ==========================
 * Raw socket code is verbose and platform-specific.
 * These helpers wrap the POSIX socket API into cleaner functions.
 *
 * Platform notes:
 *     - Linux/macOS: Use POSIX sockets (this code)
 *     - Windows: Would need Winsock (similar but different API)
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Socket type alias for potential Windows compatibility
typedef int Socket;

// Invalid socket constant
#define INVALID_SOCKET (-1)

/**
 * CONCEPT: Socket Address Structures
 * ==================================
 * struct sockaddr_in holds an IPv4 address + port:
 *
 * ┌────────────────────────────────────────┐
 * │ sin_family  │ AF_INET (IPv4)          │
 * │ sin_port    │ Port number (big-endian)│
 * │ sin_addr    │ IP address (big-endian) │
 * └────────────────────────────────────────┘
 *
 * Note: Network byte order is BIG-ENDIAN.
 * Use htons()/htonl() to convert from host byte order.
 */

/**
 * net_init - Initialize networking subsystem
 *
 * On Unix, this is a no-op.
 * On Windows, this would call WSAStartup().
 *
 * @return 0 on success, -1 on failure
 */
int net_init(void);

/**
 * net_cleanup - Cleanup networking subsystem
 *
 * On Unix, this is a no-op.
 * On Windows, this would call WSACleanup().
 */
void net_cleanup(void);

/**
 * net_create_server - Create a listening server socket
 *
 * CONCEPT: The BLAB Server Setup
 * ==============================
 * This function does steps 1-2 of BLAB:
 *     B - Bind (associate with port)
 *     L - Listen (mark as accepting connections)
 *
 * After calling this, use accept() to wait for clients.
 *
 * @param port     Port number to listen on
 * @param backlog  Max pending connections (usually 5-10)
 * @return         Socket file descriptor, or INVALID_SOCKET on error
 */
Socket net_create_server(uint16_t port, int backlog);

/**
 * net_accept_client - Accept a waiting client connection
 *
 * CONCEPT: Blocking Accept
 * ========================
 * accept() is a BLOCKING call - it waits until a client connects.
 * This is why we need threads in Module 5!
 *
 * @param server_socket  The listening socket from net_create_server
 * @param client_addr    Output: Filled with client's address info
 * @return               New socket for this specific client, or INVALID_SOCKET
 */
Socket net_accept_client(Socket server_socket, struct sockaddr_in* client_addr);

/**
 * net_connect_to_server - Connect to a remote server
 *
 * CONCEPT: Client Connection
 * ==========================
 * Creates a socket and connects to the specified server.
 * Returns when connection is established (or fails).
 *
 * @param host  IP address or hostname (e.g., "127.0.0.1")
 * @param port  Port number
 * @return      Connected socket, or INVALID_SOCKET on error
 */
Socket net_connect_to_server(const char* host, uint16_t port);

/**
 * net_send_all - Send all data, handling partial sends
 *
 * CONCEPT: Partial Sends
 * ======================
 * send() doesn't guarantee all bytes are sent at once!
 * It returns the number actually sent, which could be less than requested.
 *
 * Example:
 *     send(sock, data, 1000, 0);  // Might only send 500 bytes!
 *
 * This function loops until all data is sent.
 *
 * @param socket  Socket to send on
 * @param data    Data to send
 * @param length  Number of bytes to send
 * @return        Number of bytes sent, or -1 on error
 */
int net_send_all(Socket socket, const void* data, int length);

/**
 * net_recv_all - Receive exactly N bytes
 *
 * CONCEPT: Framing
 * ================
 * TCP is a STREAM protocol - there are no message boundaries.
 * If you send two messages back-to-back, they might arrive as:
 *     - One recv() with both messages
 *     - One recv() with half of message 1
 *     - Etc.
 *
 * This function ensures we read exactly 'length' bytes.
 *
 * @param socket  Socket to receive from
 * @param buffer  Buffer to fill
 * @param length  Exact number of bytes to receive
 * @return        Number of bytes received, 0 on disconnect, -1 on error
 */
int net_recv_all(Socket socket, void* buffer, int length);

/**
 * net_close - Close a socket
 *
 * @param socket  Socket to close
 */
void net_close(Socket socket);

/**
 * net_set_nonblocking - Make a socket non-blocking
 *
 * CONCEPT: Blocking vs Non-Blocking
 * =================================
 * By default, recv() blocks (waits) until data arrives.
 * In non-blocking mode, recv() returns immediately:
 *     - With data if available
 *     - With EAGAIN/EWOULDBLOCK if no data
 *
 * Non-blocking is useful for game loops that can't afford to wait.
 *
 * @param socket  Socket to modify
 * @return        0 on success, -1 on error
 */
int net_set_nonblocking(Socket socket);

/**
 * net_get_error_string - Get human-readable error message
 *
 * @return  String describing the last socket error
 */
const char* net_get_error_string(void);

/**
 * net_addr_to_string - Convert sockaddr_in to human-readable string
 *
 * @param addr    Address to convert
 * @param buffer  Output buffer (should be at least 22 bytes: xxx.xxx.xxx.xxx:65535)
 * @param size    Size of buffer
 * @return        Pointer to buffer, or NULL on error
 */
char* net_addr_to_string(const struct sockaddr_in* addr, char* buffer, int size);

#endif // NETWORK_H
