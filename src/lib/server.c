/*
 * Vaughn Kottler, 11/19/2018
 */

#include "server.h"

/*
 * Initialize a telemetry server.
 */
bool telemetry_server_init(telemetry_server_t *server,
                           sink_server_init_t sink_init,
                           source_server_init_t source_init,
                           sink_connection_manager_t sink_connection_handle,
                           source_connection_manager_t source_connection_handle,
                           server_fd_deinit_t close_handle)
{
    int i;
    telemetry_connection_t *curr_connection;

    server->sink_server_fd = -1;
    server->source_server_fd = -1;
    server->max_sources = TELEMETRY_MAX_SOURCES;
    server->max_sinks = TELEMETRY_MAX_SINKS;
    server->input_buffer_size = TELEMETRY_INPUT_BUFFER_SIZE;
    server->sink_connection_handle = sink_connection_handle;
    server->source_connection_handle = source_connection_handle;
    server->close_handle = close_handle;

    /* initialize file descriptors */
    server->sink_server_fd = sink_init();
    server->source_server_fd = source_init();
    if (server->sink_server_fd < 0 || server->source_server_fd < 0)
    {
        telemetry_debug("%s: couldn't get source or sink fd\r\n", __func__);
        return false;
    }

    /* initialize sources */
    for (i = 0; i < TELEMETRY_MAX_SOURCES; i++)
    {
        curr_connection = &server->sources[i];
        connection_init(curr_connection, "UNKNOWN", NULL, NULL, NULL);
    }

    /* initialize sinks */
    for (i = 0; i < TELEMETRY_MAX_SINKS; i++)
    {
        curr_connection = &server->sinks[i];
        connection_init(curr_connection, "UNKNOWN", NULL, NULL, NULL);
    }

    return true;
}

/*
 * Handle any pending packets on any currently active sources.
 */
void telemetry_server_service_sources(telemetry_server_t *server)
{
    int i;
    ssize_t amount_read;
    telemetry_connection_t *curr_connection;

    for (i = 0; i < TELEMETRY_MAX_SOURCES; i++)
    {
        curr_connection = &server->sources[i];
        if (curr_connection->state == TELEMETRY_CONNECTION_CONNECTED)
        {
            amount_read = connection_read(curr_connection,
                                          &server->input_buffer,
                                          server->input_buffer_size);
            if (amount_read < 0)
            {
                telemetry_debug("%s: read failed\r\n", curr_connection->name);
                telemetry_connection_disconnect(curr_connection, NULL);
                continue;
            }
            telemetry_server_service_sinks(server,
                                           (void *) &server->input_buffer,
                                           (size_t) amount_read);
        }
    }
}

/*
 * Send data to all currently active sinks.
 */
void telemetry_server_service_sinks(telemetry_server_t *server,
                                    const void *buffer, size_t num_bytes)
{
    int i;
    ssize_t curr_transaction;
    ssize_t bytes_sent;
    telemetry_connection_t *curr_connection;

    for (i = 0 ; i < TELEMETRY_MAX_SINKS; i++)
    {
        bool warning_printed = false;
        curr_connection = &server->sinks[i];
        bytes_sent = 0;
        if (curr_connection->state == TELEMETRY_CONNECTION_CONNECTED)
        {
            while ((size_t) bytes_sent < num_bytes)
            {
                curr_transaction = connection_write(curr_connection,
                                                    buffer,
                                                    num_bytes - bytes_sent);
                if (curr_transaction < 0)
                {
                    telemetry_debug("%s: write failed\r\n", curr_connection->name);
                    telemetry_connection_disconnect(curr_connection, NULL);
                    break;
                } 
                else if (curr_transaction == 0 && !warning_printed)
                {
                    telemetry_debug("%s: write returned 0 (buffer full?)\r\n",
                                    curr_connection->name);
                    warning_printed = true;
                }
                bytes_sent += curr_transaction;
            }
        }
    }
}

/*
 * Service connection events.
 */
void telemetry_server_service_connection_events(telemetry_server_t *server)
{
    server->source_connection_handle(server->source_server_fd,
                                     server->sources,
                                     server->max_sources);
    server->sink_connection_handle(server->sink_server_fd,
                                   server->sinks,
                                   server->max_sinks);
}

/*
 * Service connection events and pending packets.
 */
void telemetry_server_service_all(telemetry_server_t *server)
{
    telemetry_server_service_connection_events(server);
    telemetry_server_service_sources(server);
}

/*
 * Close all currently active connections and stop listening for
 * new connections.
 */
void telemetry_server_stop(telemetry_server_t *server)
{
    int i;
    telemetry_connection_t *curr_connection;

    /* stop listening for incoming connections */
    server->close_handle(server->sink_server_fd);
    server->close_handle(server->source_server_fd);

    /* close source connections */
    for (i = 0; i < TELEMETRY_MAX_SOURCES; i++)
    {
        curr_connection = &server->sources[i];
        if (curr_connection->state == TELEMETRY_CONNECTION_CONNECTED)
            telemetry_connection_disconnect(curr_connection, NULL);
    }

    /* close sink connections  */
    for (i = 0; i < TELEMETRY_MAX_SINKS; i++)
    {
        curr_connection = &server->sinks[i];
        if (curr_connection->state == TELEMETRY_CONNECTION_CONNECTED)
            telemetry_connection_disconnect(curr_connection, NULL);
    }
}
