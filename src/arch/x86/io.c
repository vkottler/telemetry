/*
 * Vaughn Kottler, 11/19/2018
 */

#include "io.h"

/*
 * Connection event handler for sockets.
 */
telemetry_connection_state_t connection_actor(telemetry_connection_t *connection,
                                              telemetry_connection_request_t request,
                                              void *param)
{
    if (request == TELEMETRY_CONNECTION_REQUEST_DISCONNECT)
    {
        socket_closer(connection->fd);
        connection->fd = -1;
        return TELEMETRY_CONNECTION_DISCONNECTED;
    }
    else if (request == TELEMETRY_CONNECTION_REQUEST_CONNECT)
    {
        return (x86_connect(connection->fd, param) < 0) ?
            TELEMETRY_CONNECTION_ERROR :
            TELEMETRY_CONNECTION_CONNECTED;
    }
    return TELEMETRY_CONNECTION_ERROR;
}

/*
 * Attempts to create and return a server socket for sink clients.
 */
int sink_server_init(void)
{
    return x86_get_server_fd(TELEMETRY_SINK_PORT, TELEMETRY_MAX_SINKS);
}

/*
 * Get a sink client file descriptor.
 */
int sink_client_init(void)
{
    return x86_get_client_fd();
} 

/*
 * Attempts to create and return a server socket for source clients.
 */
int source_server_init(void)
{
    return x86_get_server_fd(TELEMETRY_SOURCE_PORT, TELEMETRY_MAX_SOURCES);
}

/*
 * Get a source client file descriptor.
 */
int source_client_init(void)
{
    return x86_get_client_fd();
}

/*
 * Front-end for a system-defined call.
 */
int sys_fd_errors(int fd)
{
    return socket_errors(fd);
}

/*
 * Manages potential sink connections.
 */
void sink_connection_manager(int sink_server_fd,
                             telemetry_connection_t *connections,
                             size_t total_connections)
{
    unsigned int i;
    int potential_connection;
    struct sockaddr_in client_addr;
    char str[INET_ADDRSTRLEN];

    for (i = 0; i < total_connections; i++)
    {
        if (connections[i].state == TELEMETRY_CONNECTION_DISCONNECTED ||
            connections[i].state == TELEMETRY_CONNECTION_INITED)
        {
            potential_connection = x86_check_connections(sink_server_fd, &client_addr);
            if (potential_connection > 0)
            {
                if (connection_init(&connections[i], "sink_client",
                    connection_actor, socket_writer, socket_reader))
                {
                    connections[i].state = TELEMETRY_CONNECTION_CONNECTED;
                    connections[i].fd = potential_connection;
                    sprintf(connections[i].metadata, "%s:%d",
                            inet_ntop(AF_INET, &client_addr.sin_addr,
                                      str, INET_ADDRSTRLEN),
                            client_addr.sin_port);
                } else {
                    telemetry_debug("%s: error initializing connection\r\n", __func__);
                    socket_closer(potential_connection);
                }
            }
        }
    }
}

/*
 * Manages potential source connections.
 */
void source_connection_manager(int source_server_fd,
                               telemetry_connection_t *connections,
                               size_t total_connections)
{
    unsigned int i;
    int potential_connection;
    struct sockaddr_in client_addr;
    char str[INET_ADDRSTRLEN];

    for (i = 0; i < total_connections; i++)
    {
        if (connections[i].state == TELEMETRY_CONNECTION_DISCONNECTED ||
            connections[i].state == TELEMETRY_CONNECTION_INITED)
        {
            potential_connection = x86_check_connections(source_server_fd, &client_addr);
            if (potential_connection > 0)
            {
                if (connection_init(&connections[i], "source_client",
                    connection_actor, socket_writer, socket_reader))
                {
                    connections[i].state = TELEMETRY_CONNECTION_CONNECTED;
                    connections[i].fd = potential_connection;
                    sprintf(connections[i].metadata, "%s:%d",
                            inet_ntop(AF_INET, &client_addr.sin_addr,
                                      str, INET_ADDRSTRLEN),
                            client_addr.sin_port);
                } else {
                    telemetry_debug("%s: error initializing connection\r\n", __func__);
                    socket_closer(potential_connection);
                }
            }
        }
    }
}
