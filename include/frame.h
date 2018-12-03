/*
 * Vaughn Kottler, 12/02/2018
 */

#pragma once

#define TELEM_SOF   ((char) 0xDE)
#define TELEM_EOF   ((char) 0xAD)

#include "telemetry.h"

typedef enum _frame_type {
    TELEM_FRAME_INVALID = 0,
    TELEM_FRAME_CONSOLE = 1,
    TELEM_FRAME_DATA = 2,
    TELEM_FRAME_MANIFEST = 3,
} frame_type_t;

typedef struct _frame {
    frame_type_t type;
    size_t       size;
    void        *data;
} frame_t;

typedef struct _frame_handler {
    int (*console)(const char *data, size_t len);
    int (*data)(telemetry_packet_t *packet);
    int (*manifest)(const char *data, size_t len);
} frame_handler_t;

frame_t *frame_read(int fd);
int handle_frame(frame_handler_t *handler, frame_t *frame);
