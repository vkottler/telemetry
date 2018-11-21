/*
 * Vaughn Kottler, 11/20/2018
 */

#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

int x86_check_connections(int fd, struct sockaddr_in *address);
int x86_get_server_fd(int16_t port, int backlog);
int x86_get_client_fd(void);
int x86_connect(int socket, void *param);

int socket_closer(int fd);
ssize_t socket_writer(int fd, const void *buffer, size_t num_bytes);
ssize_t socket_reader(int fd, void *buffer, size_t num_bytes);
int socket_errors(int fd);
