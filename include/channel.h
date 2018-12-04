/*
 * Vaughn Kottler, 11/19/2018
 */

#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

typedef enum _channel_data {
    TELEM_UNDEF  = 0,
    TELEM_INT8   = 1,
    TELEM_UINT8  = 2,
    TELEM_INT16  = 3,
    TELEM_UINT16 = 4,
    TELEM_INT32  = 5,
    TELEM_UINT32 = 6,
    TELEM_FLOAT  = 7,
    TELEM_STRING = 8
} channel_data_t;

typedef struct _channel {
    const char     *name;
    const char     *unit;
    channel_data_t  type;
    uint32_t        size;
    void           *data;
    uint32_t        manifest_index;
} channel_t;

void channel_print(FILE *stream, channel_t *channel);
const char *channel_type_to_str(channel_data_t type);
