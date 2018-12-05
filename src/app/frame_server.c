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

#define RPI

#ifdef RPI
#include "gpio.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>

int  mem_fd;
void *gpio_map;
volatile unsigned *gpio;
struct timespec gpio_wait;

void gpio_config(void)
{
    /* open /dev/mem */
    if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
        printf("can't open /dev/gpiomem \n");
        exit(-1);
    }

    /* mmap GPIO */
    gpio_map = mmap(
        NULL,                 // Any adddress in our space will do
        BLOCK_SIZE,           // Map length
        PROT_READ|PROT_WRITE, // Enable reading & writting to mapped memory
        MAP_SHARED,           // Shared with other processes
        mem_fd,               // File to map
        GPIO_BASE             // Offset to GPIO peripheral
    );
    close(mem_fd);            // No need to keep mem_fd open after mmap

    if (gpio_map == MAP_FAILED)
    {
        printf("mmap error %d\n", (int) gpio_map);
        exit(-1);
    }

    gpio = (volatile unsigned *) gpio_map;

    gpio_wait.tv_sec = 0;
    gpio_wait.tv_nsec = 1000000;
}
#endif

int handle_console(const char *data, size_t len)
{
    write(console_fd, data, len);
    fwrite(data, 1, len, stdout);
    fflush(stdout);
    return 0;
}

int handle_data(telemetry_packet_t *packet)
{
    //puts("telemetry packet");
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
        else
        {
#ifdef RPI
            while (GET_GPIO(aux) == 0)
                nanosleep(&gpio_wait, NULL);
#endif
            if (write(serial_fd, &curr, 1) != 1)
                perror("writing to serial fd");
        }
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

#ifdef RPI
    gpio_config();
    INP_GPIO(mode_0);
    OUT_GPIO(mode_0);
    INP_GPIO(mode_1);
    OUT_GPIO(mode_1);
    GPIO_CLR = 1 << mode_0;
    GPIO_CLR = 1 << mode_1;
#endif

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
