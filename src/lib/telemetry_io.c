/*
 * Vaughn Kottler, 11/19/2018
 */

#include "telemetry.h"

#include <string.h>

#define LINE_BUFFER_LEN 256

/*
 * Send a channel manifest over a connection.
 */
bool connection_send_manifest(telemetry_connection_t *connection,
                              channel_manifest_t *manifest)
{
    uint32_t i;
    size_t line_size;
    ssize_t result;
    channel_t *channel;
    char line_buffer[LINE_BUFFER_LEN];

    if (connection->state != TELEMETRY_CONNECTION_CONNECTED)
    {
        telemetry_debug("%s: client is not connected\r\n", __func__);
        return false;
    }

    /* send all channels one-by-one */
    if (!connection_puts(connection, TELEMETRY_MANIFEST_START))
        return false;
    for (i = 0; i < manifest->count; i++)
    {
        channel = &manifest->channels[i];
        line_size = channel_manifest_line_size(channel);
        if (line_size > LINE_BUFFER_LEN)
        {
            return false;
        }
        sprintf(line_buffer, "%s %s %s %u\r\n", channel->name, channel->unit,
                channel_type_to_str(channel->type), channel->size);
        result = connection_write(connection, line_buffer, line_size);
        if (result < 0 || (size_t) result != line_size)
        {
            telemetry_debug("%s: error sending manifest line '%s'\r\n",
                            __func__, line_buffer);
            return false;
        }
    }
    if (!connection_puts(connection, TELEMETRY_MANIFEST_END))
        return false;

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
                        void *buffer, size_t num_bytes)
{
    ssize_t result;
    if (connection == NULL || connection->fd < 0) return -1;
    result = connection->read_handle(connection->fd, buffer, num_bytes);
    if (result < 0)
    {
        telemetry_debug("%s: a read failed, disconnecting\r\n", __func__);
        telemetry_connection_disconnect(connection, NULL);
    }
    return result;
}

/*
 * Attempt to write to a connection.
 */
ssize_t connection_write(telemetry_connection_t *connection,
                         const void *buffer, size_t num_bytes)
{
    ssize_t result;
    if (connection == NULL || connection->fd < 0) return -1;
    result = connection->write_handle(connection->fd, buffer, num_bytes);
    if (result < 0)
    {
        telemetry_debug("%s: a write failed, disconnecting\r\n", __func__);
        telemetry_connection_disconnect(connection, NULL);
    }
    return result;
}

bool connection_puts(telemetry_connection_t *connection, const char *str)
{
    size_t str_len = strlen(str);
    return ((size_t) connection_write(connection, str, str_len) == str_len);
}
