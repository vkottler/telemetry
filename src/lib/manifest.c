/*
 * Vaughn Kottler, 10/18/2018
 */

#include "telemetry.h"

#include <stdlib.h>

/*
 * Create a channel manifest with a specified initial capacity.
 */
channel_manifest_t *channel_manifest_create(uint32_t capacity)
{
#ifdef TELEMETRY_NO_SYS
    telemetry_debug("%s: can't dynamically allocate a manifest\r\n", __func__);
    return NULL;
#else
    channel_manifest_t *manifest = malloc(sizeof(channel_manifest_t));
    if (!manifest)
    {
        telemetry_debug("%s: malloc failed\r\n", __func__);
        return NULL;
    }
    manifest->count = 0;
    manifest->channels = calloc(capacity, sizeof(channel_t));
    if (!manifest->channels)
    {
        free(manifest);
        telemetry_debug("%s: calloc failed\r\n", __func__);
        return NULL;
    }
    manifest->capacity = capacity;
    return manifest;
#endif
}

/*
 * Print a channel manifest to the given IO stream.
 */
void channel_manifest_print(FILE *stream, channel_manifest_t *manifest)
{
    fputs("********************\r\n", stream);
    fprintf(stream, "Count:    %lu\r\n", manifest->count);
    fprintf(stream, "Capacity: %lu\r\n", manifest->capacity);
    for (unsigned int i = 0; i < manifest->count; i++)
        channel_print(stream, &manifest->channels[i]);
    fputs("********************\r\n", stream);
}

void channel_manifest_send(channel_manifest_t *telem_manifest, void (*send_func_ptr)(char *, uint32_t))
{
    char char_buffer[256];
    for (unsigned int i = 0; i < telem_manifest->count; i++) {
        channel_t *channel = &telem_manifest->channels[i];
        *((uint32_t *) char_buffer) = channel->manifest_index;
        *((channel_data_t *) &char_buffer[sizeof(uint32_t)]) = channel->type;
        *((size_t *) &char_buffer[sizeof(uint32_t) + sizeof(channel_data_t)]) = channel->size;
        sprintf(&char_buffer[sizeof(uint32_t) + sizeof(channel_data_t) + sizeof(size_t)], "%s,%s\n",
                channel->name, channel->unit);

        //Send the buffer
        send_func_ptr(char_buffer, sizeof(char_buffer));
    }
}
