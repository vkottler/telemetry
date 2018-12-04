/*
 * Vaughn Kottler, 12/02/2018
 */

#include "telemetry.h"

#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#define UNUSED(x) ((void)(x))

#define MANIFEST_HOST   "localhost"
#define MANIFEST_PORT   5000
#define DATA_HOST       "localhost"
#define DATA_PORT       6000
#define CONSOLE_HOST    "localhost"
#define CONSOLE_PORT    9020

#define COMMAND_PORT    10055

int console_fd = -1;
int data_fd = -1;
int manifest_fd = -1;
int serial_fd = -1;

struct timespec sleep_time;

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

int create_server_fd(uint16_t port, int backlog)
{
    int fd;
    int option_value = 1;
    struct sockaddr_in address;

    /* create a TCP socket set up for non-blocking interactions */
    if ((fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        perror("socket creation");
        return fd;
    }

    /* allow rapid re-use of address and port pair */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
    &option_value, sizeof(int)))
    {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    /* request the system to let us service this port on any of our
    *      * network interfaces */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(fd, (const struct sockaddr *) &address,
        sizeof(struct sockaddr_in)))
    {
        perror("bind");
        close(fd);
        return -1;
    }

    /* tell the system we want to passively queue incoming client
    *      * connections and service them through 'accept' or other calls  */
    if (listen(fd, backlog))
    {
        perror("listen");
        close(fd);
        return -1;
    }

    printf("%s: listening on %d\r\n", __func__, port);

    return fd;
}


/*
 *  * A function handle for continuously polling a 'bound' and 'listening'
 *   * socket. This will return a file descriptor of a new client socket if
 *    * one is found.
 *     */
int check_connections(int fd, struct sockaddr_in *address)
{
    char str[INET_ADDRSTRLEN];
    socklen_t sock_len = sizeof(struct sockaddr_in);
    int flags;
    int potential_connection = accept(fd, (struct sockaddr *) address,
                                      &sock_len);
    if (potential_connection < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        perror("accept");
    else if (potential_connection > 0)
    {
        flags = fcntl(potential_connection, F_GETFL, 0);
        fcntl(potential_connection, F_SETFL, flags | O_NONBLOCK);
        printf("%s:%d connected\r\n", inet_ntop(AF_INET, &address->sin_addr,
                                                str, INET_ADDRSTRLEN),
               address->sin_port);
        return potential_connection;
    }
    return -1;
}

void *command_server(void *arg)
{
    char str[INET_ADDRSTRLEN];
    int server;
    int client_fd = -1;
    struct sockaddr_in client_address;
    ssize_t read_result;
    char curr;

    UNUSED(arg);

    server = create_server_fd(COMMAND_PORT, 1);

    while (run)
    {
        /* await incoming clients */
        if (client_fd == -1)
        {
            client_fd = check_connections(server, &client_address);
            inet_ntop(AF_INET, &client_address.sin_addr, str, INET_ADDRSTRLEN);
        }
        /* handle current client */
        else
        {
            read_result = read(client_fd, &curr, 1);
            /* client is closing the connection */
            if (read_result == 0)
            {
                printf("%s:%d disconnected\r\n", str, client_address.sin_port);
                close_socket(client_fd);
                client_fd = -1;
            }
            /* nothing to read, or an error occurred */
            else if (read_result < 0)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    perror("client connection read");
                    printf("closing client %s:%d\r\n", str, client_address.sin_port);
                    close_socket(client_fd);
                    client_fd = -1;
                }
            }
            /* a byte was read */
            else if (write(serial_fd, &curr, 1) != 1)
                perror("writing to serial fd");
        }
        nanosleep(&sleep_time, NULL);
    }

    /* close any current client's connection */
    if (client_fd != -1)
    {
        printf("closing client %s:%d\r\n", str, client_address.sin_port);
        close_socket(client_fd);
    }
    close(server);

    return NULL;
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
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("console client connection read");
                close_socket(console_fd);
                return NULL;
            }
        }
        /* a byte was read */
        else if (write(serial_fd, &curr, 1) != 1)
            perror("writing to serial fd");
        nanosleep(&sleep_time, NULL);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t server_thread, console_client_reader_thread;
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
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 50000;

    /* get serial fd */
    serial_fd = open(argv[1], O_RDWR | O_NOCTTY);
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
        close(serial_fd);
        error_msg = "couldn't establish a client connection";
        goto report_error_quit;
    }

    /* start command server thread */
    server_thread = pthread_create(&server_thread, NULL, command_server, NULL);
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
    pthread_join(server_thread, NULL);
    pthread_join(console_client_reader_thread, NULL);

    return 0;

report_error_quit:
    fprintf(stderr, "%s: %s\r\n", __func__, error_msg);
    return 1;
}
