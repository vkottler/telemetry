/*
 * Vaughn Kottler, 11/19/2018
 */

#include "telemetry.h"

/*
 * Send a channel manifest over a connection.
 */
bool connection_send_manifest(telemetry_connection_t *connection,
                              channel_manifest_t *manifest)
{
    if (connection->state != TELEMETRY_CONNECTION_CONNECTED)
    {
        telemetry_debug("%s: client is not connected\r\n", __func__);
        return false;
    }

    /* send manifest */
    ((void)(manifest));

    return true;
}

/*
 * Send a packet over a connection.
 */
bool connection_send_packet(telemetry_connection_t *connection,
                            telemetry_packet_t *packet)
{
    if (connection->state != TELEMETRY_CONNECTION_CONNECTED)
    {
        telemetry_debug("%s: client is not connected\r\n", __func__);
        return false;
    }

    /* set crc32, send packet */
    ((void)(packet));

    return true;
}

/*
 * Attempt to read from a connection.
 */
ssize_t connection_read(telemetry_connection_t *connection,
                        const void *buffer, size_t num_bytes)
{
    if (connection == NULL || connection->fd < 0) return -1;
    return connection->read_handle(connection->fd, buffer, num_bytes);
}

/*
 * Attempt to write to a connection.
 */
ssize_t connection_write(telemetry_connection_t *connection,
                         const void *buffer, size_t num_bytes)
{
    if (connection == NULL || connection->fd < 0) return -1;
    return connection->write_handle(connection->fd, buffer, num_bytes);
}
