/*
 * Vaughn Kottler, 12/04/2018
 */

#pragma once

#include <stdint.h>

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define MANIFEST_HOST   "localhost"
#define MANIFEST_PORT   5000
#define DATA_HOST       "localhost"
#define DATA_PORT       6000
#define CONSOLE_HOST    "localhost"
#define CONSOLE_PORT    9020

#define PING_STR        "ping\r\nping\r\nping\r\n"

int close_socket(int fd);
int client_connect(int fd, const char *host, uint16_t port);
int get_socket(void);
