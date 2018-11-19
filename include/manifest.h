/*
 * Vaughn Kottler, 11/19/2018
 */

#pragma once

#include "channel.h"

typedef struct _channel_manifest {
    channel_t *channels;
    uint32_t   count;
    uint32_t   capacity;
} channel_manifest_t;

uint32_t channel_add(channel_manifest_t *manifest,
                     const char *name, const char *unit,
                     channel_data_t type, size_t size);

channel_manifest_t *channel_manifest_create(uint32_t capacity);
void channel_manifest_print(FILE *stream, channel_manifest_t *manifest);
