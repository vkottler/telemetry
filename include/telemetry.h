/*
 * Vaughn Kottler, 10/17/2018
 */

#pragma once

#include "packet.h"

#include <stdbool.h>
#include <sys/types.h>

#define TELEMETRY_CAPACITY      64
#define TELEMETRY_MTU           1500
#define telemetry_debug         printf
#define TELEMETRY_METADATA_SIZE 32

typedef enum _telemetry_connection_state {
    TELEMETRY_CONNECTION_ERROR = 0,
    TELEMETRY_CONNECTION_INITED = 1,
    TELEMETRY_CONNECTION_CONNECTED = 2,
    TELEMETRY_CONNECTION_DISCONNECTED = 3,
} telemetry_connection_state_t;
const char *connection_state_to_str(telemetry_connection_state_t state);

typedef enum _telemetry_connection_request {
    TELEMETRY_CONNECTION_REQUEST_RESERVED = 0,
    TELEMETRY_CONNECTION_REQUEST_CONNECT = 1,
    TELEMETRY_CONNECTION_REQUEST_DISCONNECT = 2,
} telemetry_connection_request_t;
const char *connection_request_to_str(telemetry_connection_request_t request);

typedef ssize_t (*write_handle_t)(int, const void *, size_t);
typedef ssize_t (*read_handle_t)(int, void *, size_t);

typedef struct _telemetry_connection {
    int fd;
    telemetry_connection_state_t state;
    const char *name;
    /* same as 'connection_handle_t', can't forward-declare */
    telemetry_connection_state_t (*connection_handle)(struct _telemetry_connection *, 
                                                      telemetry_connection_request_t,
                                                      void *);
    write_handle_t write_handle;
    read_handle_t read_handle;
    char metadata[TELEMETRY_METADATA_SIZE];
} telemetry_connection_t;

typedef telemetry_connection_state_t (*connection_handle_t)(telemetry_connection_t *,
                                                            telemetry_connection_request_t,
                                                            void *);

/*****************************************************************************/

telemetry_connection_state_t connection_change_state(telemetry_connection_t *connection,
                                                     telemetry_connection_request_t request,
                                                     void *param);
bool telemetry_connection_connect(telemetry_connection_t *connection, void *param);
bool telemetry_connection_disconnect(telemetry_connection_t *connection, void *param);
bool connection_init(telemetry_connection_t *connection, const char *name,
                     connection_handle_t connection_handle,
                     write_handle_t write_handle, read_handle_t read_handle);

bool connection_send_manifest(telemetry_connection_t *connection,
                              channel_manifest_t *manifest);
bool connection_send_packet(telemetry_connection_t *connection,
                            telemetry_packet_t *packet);

ssize_t connection_read(telemetry_connection_t *connection,
                        void *buffer, size_t num_bytes);
ssize_t connection_write(telemetry_connection_t *connection,
                         const void *buffer, size_t num_bytes);
int connection_errors(telemetry_connection_t *connection);
