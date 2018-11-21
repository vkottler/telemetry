/*
 * Vaughn Kottler, 11/19/2018
 */

#pragma once

#include "server.h"
#include "socket.h"

#include <signal.h>

/* telemetry proxy handles */

int sink_server_init(void);
int sink_client_init(void);
int source_server_init(void);
int source_client_init(void);
void *get_client_param(const char *name, int16_t port);
extern int sys_fd_errors(int fd);
void sink_connection_manager(int sink_server_fd,
                             telemetry_connection_t *connections,
                             size_t total_connections);
void source_connection_manager(int source_server_fd,
                               telemetry_connection_t *connections,
                               size_t total_connections);
telemetry_connection_state_t connection_actor(telemetry_connection_t *connection,
                                              telemetry_connection_request_t request,
                                              void *param);
