/*
 * Vaughn Kottler, 12/02/2018
 */

#include "telemetry.h"
#include "apps.h"

#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

int console_fd = -1;
int data_fd = -1;
int manifest_fd = -1;
int serial_fd = -1;

int handle_console(const char *data, size_t len)
{
    write(console_fd, data, len);
    fwrite(data, 1, len, stdout);
    fflush(stdout);
    return 0;
}

int handle_data(telemetry_packet_t *packet)
{
    puts("telemetry packet");
    write(data_fd, packet, telemetry_packet_size(packet));
    return 0;
}

int handle_manifest(const char *data, size_t len)
{
    write(manifest_fd, data, len);
    fwrite(data, 1, len, stdout);
    fflush(stdout);
    return 0;
}

volatile int run = 1;
void sigint_handler(int signum)
{
    UNUSED(signum);
    run = 0;
}

void configure_serial_fd(int fd, speed_t baud)
{
    struct termios options;
    cfsetospeed (&options, baud);
    cfsetispeed (&options, baud);
    tcgetattr(fd, &options);
    options.c_cflag = (options.c_cflag & ~CSIZE) | CS8;
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_iflag &= ~IGNBRK;
    options.c_iflag |= IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &options);
}

void *console_client_reader(void *arg)
{
    char curr;
    ssize_t read_result;
    UNUSED(arg);
    while (run)
    {
        read_result = read(console_fd, &curr, 1);
        if (read_result == 0)
        {
            puts("console client closed connection");
            return NULL;
        }
        /* nothing to read, or an error occurred */
        else if (read_result < 0)
        {
            perror("console client connection read");
            close_socket(console_fd);
            return NULL;
        }
        /* a byte was read */
        else if (write(serial_fd, &curr, 1) != 1)
            perror("writing to serial fd");
    }
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t console_client_reader_thread;
    frame_handler_t frame_handler;
    frame_t *curr_frame;
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
    serial_fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (serial_fd == -1)
    {
        error_msg = "couldn't open serial stream";
        goto report_error_quit;
    }
    configure_serial_fd(serial_fd, B115200);

    /* connect to clients */
    console_fd = get_socket();
    data_fd = get_socket();
    manifest_fd = get_socket();
    if (client_connect(console_fd, CONSOLE_HOST, CONSOLE_PORT) ||
        client_connect(data_fd, DATA_HOST, DATA_PORT) ||
        client_connect(manifest_fd, MANIFEST_HOST, MANIFEST_PORT))
    {
        close(serial_fd);
        error_msg = "couldn't establish a client connection";
        goto report_error_quit;
    }

    /* start command server thread */
    console_client_reader_thread = pthread_create(&console_client_reader_thread,
                                                  NULL, console_client_reader, NULL);

    /* handle incoming frames until the user cancels execution with Ctrl-C */
    signal(SIGINT, sigint_handler);
    while (run)
    {
        curr_frame = frame_read(serial_fd);
        if (curr_frame == NULL) continue;
        if (handle_frame(&frame_handler, curr_frame))
        {
            error_msg = "handle_frame failed";
            fprintf(stderr, "%s: %s\r\n", __func__, error_msg);
        }
    }

    /* close serial fd */
    close(serial_fd);

    /* close client connections */
    close_socket(console_fd);
    close_socket(data_fd);
    close_socket(manifest_fd);

    /* stop command server */
    pthread_join(console_client_reader_thread, NULL);

    return 0;

report_error_quit:
    fprintf(stderr, "%s: %s\r\n", __func__, error_msg);
    return 1;
}
