/*
 * Vaughn Kottler, 10/18/2018
 */

#include "telemetry.h"

#include <stdlib.h>
#include <string.h>

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

size_t channel_manifest_line_size(channel_t *channel)
{
    size_t result = 0;
    char size_str[12]; // all 32-bit values (i.e. 'size_t') can fit in this
    result += strlen(channel->name);
    result += 1; // ' '
    result += strlen(channel->unit);
    result += 1; // ' '
    result += strlen(channel_type_to_str(channel->type));
    result += 1; // ' '
    sprintf(size_str, "%u", channel->size);
    result += strlen(size_str);
    result += 2; // '\r\n'
    return result;
}

/*
 * Print a channel manifest to the given IO stream.
 */
void channel_manifest_print(FILE *stream, channel_manifest_t *manifest)
{
    fputs("********************\r\n", stream);
    fprintf(stream, "Count:    %u\r\n", manifest->count);
    fprintf(stream, "Capacity: %u\r\n", manifest->capacity);
    for (unsigned int i = 0; i < manifest->count; i++)
        channel_print(stream, &manifest->channels[i]);
    fputs("********************\r\n", stream);
}
