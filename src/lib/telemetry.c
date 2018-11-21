/*
 * Vaughn Kottler, 10/17/2018
 */

#include "telemetry.h"

#include <string.h>

/*
 * Convert 'telemetry_connection_state_t' to a String. 
 */
const char *connection_state_to_str(telemetry_connection_state_t state)
{
    switch (state)
    {
        case TELEMETRY_CONNECTION_ERROR: return "ERROR";
        case TELEMETRY_CONNECTION_INITED: return "INITED";
        case TELEMETRY_CONNECTION_CONNECTED: return "CONNECTED";
        case TELEMETRY_CONNECTION_DISCONNECTED: return "DISCONNECTED";
    }
    return "UNKNOWN";
}

/*
 * Convert 'telemetry_connection_request_t' to a String. 
 */
const char *connection_request_to_str(telemetry_connection_request_t request)
{
    switch (request)
    {
        case TELEMETRY_CONNECTION_REQUEST_RESERVED: return "RESERVED";
        case TELEMETRY_CONNECTION_REQUEST_CONNECT: return "CONNECT";
        case TELEMETRY_CONNECTION_REQUEST_DISCONNECT: return "DISCONNECT";
    }
    return "UNKNOWN";
}

/*
 * Initialize a 'telemetry_connection_t' struct.
 */
bool connection_init(telemetry_connection_t *connection, const char *name,
                     connection_handle_t connection_handle,
                     write_handle_t write_handle, read_handle_t read_handle)
{
    if (connection == NULL || name == NULL)
    {
        telemetry_debug("%s: invalid input arguments\r\n", __func__);
        return false;
    }
    connection->name = name;
    connection->connection_handle = connection_handle;
    connection->write_handle = write_handle;
    connection->read_handle = read_handle;
    connection->state = TELEMETRY_CONNECTION_INITED;
    connection->fd = -1;
    memset(connection->metadata, 0, TELEMETRY_METADATA_SIZE);
    strcpy(connection->metadata, "UNKNOWN");
    return true;
}

/*
 * Perform a connection state transition for a given connection and requested
 * state.
 */
telemetry_connection_state_t connection_change_state(telemetry_connection_t *connection,
                                                     telemetry_connection_request_t request,
                                                     void *param)
{
    if (connection == NULL || connection->connection_handle == NULL)
    {
        telemetry_debug("%s: invalid input arguments\r\n", __func__);
        return TELEMETRY_CONNECTION_ERROR;
    }

    if (connection->state == TELEMETRY_CONNECTION_ERROR)
    {
        telemetry_debug("%s: connection already in error state\r\n", __func__);
        return TELEMETRY_CONNECTION_ERROR;
    }

    telemetry_connection_state_t initial, result;
    initial = connection->state;
    result  = connection->connection_handle(connection, request, param);
    connection->state = result;
    telemetry_debug("%s: %s --(%s)--> %s (%s)\r\n", connection->name,
                    connection_state_to_str(initial),
                    connection_request_to_str(request),
                    connection_state_to_str(result),
                    connection->metadata);
    return result;
}

/*
 * Attempt to establish an initialized connection.
 */
bool telemetry_connection_connect(telemetry_connection_t *connection, void *param)
{
    if (connection->state != TELEMETRY_CONNECTION_INITED &&
        connection->state != TELEMETRY_CONNECTION_DISCONNECTED)
    {
        telemetry_debug("%s: can only connect from %s or %s, have %s\r\n",
                        __func__,
                        connection_state_to_str(TELEMETRY_CONNECTION_INITED),
                        connection_state_to_str(TELEMETRY_CONNECTION_DISCONNECTED),
                        connection_state_to_str(connection->state));
        return false;
    }
    return (connection_change_state(connection,
                                    TELEMETRY_CONNECTION_REQUEST_CONNECT,
                                    param) == 
            TELEMETRY_CONNECTION_CONNECTED);
}

/*
 * Attempt to disconnect a currently connected connection.
 */
bool telemetry_connection_disconnect(telemetry_connection_t *connection, void *param)
{
    if (connection->state != TELEMETRY_CONNECTION_CONNECTED)
    {
        telemetry_debug("%s: can only disconnect from %s\r\n", __func__,
                        connection_state_to_str(TELEMETRY_CONNECTION_CONNECTED));
        return false;
    }
    return (connection_change_state(connection,
                                    TELEMETRY_CONNECTION_REQUEST_DISCONNECT,
                                    param) == 
            TELEMETRY_CONNECTION_DISCONNECTED);
}

/*
 * Check for error codes on a connection. The target system must provide
 * meanings for codes and implementation of retrieving errors from a file
 * descriptor.
 */
extern int sys_fd_errors(int fd);
int connection_errors(telemetry_connection_t *connection)
{
    return sys_fd_errors(connection->fd);
}
