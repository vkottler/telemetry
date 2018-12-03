/*
 * Vaughn Kottler, 12/02/2018
 */

#include "telemetry.h"

#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define UNUSED(x) ((void)(x))

#define MANIFEST_HOST   "localhost"
#define MANIFEST_PORT   5000
#define DATA_HOST       "localhost"
#define DATA_PORT       6000
#define CONSOLE_HOST    "localhost"
#define CONSOLE_PORT    9020

int console_fd = -1;
int data_fd = -1;
int manifest_fd = -1;

int handle_console(const char *data, size_t len)
{
    UNUSED(data);
    UNUSED(len);
    puts("got console data");
    return 0;
}

int handle_data(telemetry_packet_t *packet)
{
    UNUSED(packet);
    puts("got telemetry data");
    return 0;
}

int handle_manifest(const char *data, size_t len)
{
    UNUSED(data);
    UNUSED(len);
    puts("got manifest data");
    return 0;
}

volatile int run = 1;
void sigint_handler(int signum)
{
    UNUSED(signum);
    run = 0;
}

/*
 * Prevent future writes to a socket and wait for the system to return a 
 * read of zero before closing.
 */
int close_socket(int fd)
{
    char buf[64];
    shutdown(fd, SHUT_WR);
    while (read(fd, buf, 64) > 0) {;}
    return close(fd);
}

void configure_serial_fd(int fd, speed_t baud)
{
    struct termios options;
    tcgetattr(fd, &options);
    options.c_cflag = baud | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &options);
}

int client_connect(int fd, const char *host, uint16_t port)
{
    struct sockaddr_in socket_address;
    memset(&socket_address, 0, sizeof(struct sockaddr_in));
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);
    inet_pton(AF_INET, host, &socket_address.sin_addr);
    return connect(fd, (struct sockaddr *) &socket_address,
                   sizeof(struct sockaddr));
}

int main(int argc, char **argv)
{
    frame_handler_t frame_handler;
    frame_t *curr_frame;
    int serial_fd;
    int retval = 0;
    const char *error_msg = "error not specified";

    if (argc < 2)
    {
        error_msg = "usage: frame_server <serial_path>";
        goto report_error_quit;
    }

    frame_handler.console = handle_console;
    frame_handler.data = handle_data;
    frame_handler.manifest = handle_manifest;

    /* get serial fd */
    serial_fd = open(argv[1], O_RDONLY | O_NOCTTY);
    if (serial_fd == -1)
    {
        error_msg = "couldn't open serial stream";
        goto report_error_quit;
    }
    configure_serial_fd(serial_fd, B115200);

    /* connect to clients */
    console_fd = socket(AF_INET, SOCK_STREAM, 0);
    data_fd = socket(AF_INET, SOCK_STREAM, 0);
    manifest_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_connect(console_fd, CONSOLE_HOST, CONSOLE_PORT) ||
        client_connect(data_fd, DATA_HOST, DATA_PORT) ||
        client_connect(manifest_fd, MANIFEST_HOST, MANIFEST_PORT))
    {
        error_msg = "couldn't establish a client connection";
        goto report_error_quit;
    }

    /* handle incoming frames until the user cancels execution with Ctrl-C */
    signal(SIGINT, sigint_handler);
    while (run)
    {
        curr_frame = frame_read(serial_fd);
        if (handle_frame(&frame_handler, curr_frame))
        {
            error_msg = "handle_frame failed";
            goto report_error_quit;
        }
    }

    /* close serial fd */
    close(serial_fd);

    /* close client connections */
    close_socket(console_fd);
    close_socket(data_fd);
    close_socket(manifest_fd);

    return retval;

report_error_quit:
    fprintf(stderr, "%s: %s\r\n", __func__, error_msg);
    return 1;
}
