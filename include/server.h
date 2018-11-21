/*
 * Vaughn Kottler, 11/19/2018
 */

#pragma once

#include "telemetry.h"

#define TELEMETRY_MAX_SOURCES         4
#define TELEMETRY_MAX_SINKS           4
#define TELEMETRY_INPUT_BUFFER_SIZE   1024

#define TELEMETRY_SOURCE_PORT         10020
#define TELEMETRY_SINK_PORT           9002

typedef int (*sink_server_init_t)(void);
typedef int (*source_server_init_t)(void);

typedef int (*server_fd_deinit_t)(int);

typedef void (*sink_connection_manager_t)(int, telemetry_connection_t *, size_t);
typedef void (*source_connection_manager_t)(int, telemetry_connection_t *, size_t);

typedef struct _telemetry_server {
    int sink_server_fd;
    int source_server_fd;
    telemetry_connection_t sources[TELEMETRY_MAX_SOURCES];
    telemetry_connection_t sinks[TELEMETRY_MAX_SINKS];
    size_t max_sources, max_sinks, input_buffer_size;
    sink_connection_manager_t sink_connection_handle;
    source_connection_manager_t source_connection_handle;
    server_fd_deinit_t close_handle;
    char input_buffer[TELEMETRY_INPUT_BUFFER_SIZE];
} telemetry_server_t;

bool telemetry_server_init(telemetry_server_t *server,
                           sink_server_init_t sink_init,
                           source_server_init_t source_init,
                           sink_connection_manager_t sink_connection_handle,
                           source_connection_manager_t source_connection_handle,
                           server_fd_deinit_t close_handle);

void telemetry_server_service_sources(telemetry_server_t *server);
void telemetry_server_service_sinks(telemetry_server_t *server,
                                    const void *buffer, size_t num_bytes);
void telemetry_server_service_connection_events(telemetry_server_t *server);
void telemetry_server_service_all(telemetry_server_t *server);
void telemetry_server_stop(telemetry_server_t *server);
