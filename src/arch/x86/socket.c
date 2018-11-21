/*
 * Vaughn Kottler, 11/20/2018
 */

#include "socket.h"

#include <stdio.h>

/*
 * Generic function for closing network sockets, lets the system do anything
 * it might need to. Output and errno doesn't necessarily have meaning or
 * value for the caller.
 */
int socket_closer(int fd)
{
    return shutdown(fd, SHUT_RDWR) + close(fd);
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
    int potential_connection = accept(fd, (struct sockaddr *) address,
                                      &sock_len);
    if (potential_connection < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        perror("accept");
    }
    else if (potential_connection > 0)
    {
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
 * System call front-end.
 */
ssize_t socket_writer(int fd, const void *buffer, size_t num_bytes)
{
    return write(fd, buffer, num_bytes);
}

/*
 * System call front-end.
 */
ssize_t socket_reader(int fd, void *buffer, size_t num_bytes)
{
    return read(fd, buffer, num_bytes);
}
