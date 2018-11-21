/*
 * Vaughn Kottler, 11/20/2018
 */

#include "socket.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

/*
 * Generic function for closing network sockets, lets the system do anything
 * it might need to. Output and errno doesn't necessarily have meaning or
 * value for the caller.
 */
int socket_closer(int fd)
{
    char buf[64];
    int flags;
    shutdown(fd, SHUT_WR);
    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    while (read(fd, buf, 64) > 0) {;}
    return close(fd);
}

/*
 * A function handle for continuously polling a 'bound' and 'listening'
 * socket. This will return a file descriptor of a new client socket if
 * one is found.
 */
int x86_check_connections(int fd, struct sockaddr_in *address)
{
    char str[INET_ADDRSTRLEN];
    socklen_t sock_len = sizeof(struct sockaddr_in);
    int flags;
    int potential_connection = accept(fd, (struct sockaddr *) address,
                                      &sock_len);
    if (potential_connection < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        perror("accept");
    }
    else if (potential_connection > 0)
    {
        flags = fcntl(potential_connection, F_GETFL, 0);
        fcntl(potential_connection, F_SETFL, flags | O_NONBLOCK);
        printf("%s:%d connected\r\n", inet_ntop(AF_INET, &address->sin_addr,
                                                str, INET_ADDRSTRLEN),
               address->sin_port);
        return potential_connection;
    }
    return -1;
}

/*
 * Create and return a file descriptor for a 'bound' and 'listening' socket
 * for a provided port. The system will queue up 'backlog' number of pending
 * client connections.
 */
int x86_get_server_fd(int16_t port, int backlog)
{
    int fd;
    int option_value = 1;
    struct sockaddr_in address;

    /* create a TCP socket set up for non-blocking interactions */
    if ((fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        perror("socket creation");
        return fd;
    }

    /* allow rapid re-use of address and port pair */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &option_value, sizeof(int)))
    {
        perror("setsockopt");
        socket_closer(fd);
        return -1;
    }

    /* request the system to let us service this port on any of our
     * network interfaces */
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(port); 
    if (bind(fd, (const struct sockaddr *) &address,
             sizeof(struct sockaddr_in)))
    {
        perror("bind");
        socket_closer(fd);
        return -1;
    }

    /* tell the system we want to passively queue incoming client
     * connections and service them through 'accept' or other calls  */
    if (listen(fd, backlog))
    {
        perror("listen");
        socket_closer(fd);
        return -1;
    }

    printf("%s: listening on %d\r\n", __func__, port);

    return fd;
}

/*
 * Retrieve a client socket from the system.
 */
int x86_get_client_fd(void)
{
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket creation");
    return fd;
}

/*
 * Get a usable sockaddr_in struct from the given parameters.
 */
struct sockaddr_in socket_address; 
void *get_client_param(const char *name, int16_t port)
{
    memset(&socket_address, 0, sizeof(struct sockaddr_in));
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);
    inet_pton(AF_INET, name, &socket_address.sin_addr);
    return &socket_address;
}

/*
 * System call front-end.
 */
int x86_connect(int socket, void *param)
{
    return connect(socket, (struct sockaddr *) param, sizeof(struct sockaddr));
}

/*
 * System call front-end.
 */
ssize_t socket_writer(int fd, const void *buffer, size_t num_bytes)
{
    return write(fd, buffer, num_bytes);
}

/*
 * Check if there are any errors on the socket.
 */
int socket_errors(int fd)
{
    int error_code;
    socklen_t error_code_size = sizeof(error_code);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
    return error_code;
}

/*
 * System call front-end, handle non-blocking calls.
 */
ssize_t socket_reader(int fd, void *buffer, size_t num_bytes)
{
    ssize_t result = read(fd, buffer, num_bytes);
    if ((result < 0 && errno == EAGAIN) || (result < 0 && errno == EWOULDBLOCK))
        result = 0;
    return result;
}
