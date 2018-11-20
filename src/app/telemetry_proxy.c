#include <stdio.h>

#include "server.h"

#define UNUSED(x) ((void)(x))

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>

void socket_closer(int fd)
{
    if (close(fd))
    {
        perror("error closing file descriptor");
    }
}

telemetry_connection_state_t connection_closer(telemetry_connection_t *connection,
                                               telemetry_connection_request_t request,
                                               int *fd)
{
    UNUSED(connection);
    UNUSED(request);
    UNUSED(fd);

    return TELEMETRY_CONNECTION_REQUEST_RESERVED;
}

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

int sink_server_init(void)
{
    /* create a server socket for telemetry subscribers */
    return x86_get_server_fd(TELEMETRY_SINK_PORT, TELEMETRY_MAX_SINKS);
}

int source_server_init(void)
{
    /* create a server socket for telemetry publishers */
    return x86_get_server_fd(TELEMETRY_SOURCE_PORT, TELEMETRY_MAX_SOURCES);
}

void sink_connection_manager(int sink_server_fd,
                             telemetry_connection_t *connections,
                             size_t total_connections)
{
    unsigned int i;
    int potential_connection;
    struct sockaddr_in client_addr;

    for (i = 0; i < total_connections; i++)
    {
        if (connections[i].state == TELEMETRY_CONNECTION_DISCONNECTED)
        {
            potential_connection = x86_check_connections(sink_server_fd, &client_addr);
            if (potential_connection > 0)
            {
                connections[i].state = TELEMETRY_CONNECTION_CONNECTED;
                connections[i].fd = potential_connection;
                connections[i].name = "sink_client";
                connections[i].connection_handle = connection_closer;
                /* set up sink client */
            }
        }
    }
}

void source_connection_manager(int source_server_fd,
                               telemetry_connection_t *connections,
                               size_t total_connections)
{
    unsigned int i;
    int potential_connection;
    struct sockaddr_in client_addr;

    for (i = 0; i < total_connections; i++)
    {
        if (connections[i].state == TELEMETRY_CONNECTION_DISCONNECTED)
        {
            potential_connection = x86_check_connections(source_server_fd, &client_addr);
            if (potential_connection > 0)
            {
                connections[i].state = TELEMETRY_CONNECTION_CONNECTED;
                connections[i].fd = potential_connection;
                connections[i].name = "source_client";
                connections[i].connection_handle = connection_closer;
                /* set up source client */
            }
        }
    }
}

volatile bool run = true;

void sigint_handler(int signum)
{
    UNUSED(signum);
    run = false;
}

int main(int argc, char **argv)
{
    telemetry_server_t server;

	UNUSED(argc);
	UNUSED(argv);

    signal(SIGINT, sigint_handler);

    if (!telemetry_server_init(&server, sink_server_init, source_server_init, 
                               sink_connection_manager,
                               source_connection_manager, socket_closer))
    {
        fprintf(stderr, "%s: couldn't initialize server\r\n", __func__);
        return 1;
    }

    while (run)
    {
        telemetry_server_service_all(&server);
    }

    telemetry_server_stop(&server);

	return 0;
}
