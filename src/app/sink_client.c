#include "io.h"

#include <stdio.h>

#define UNUSED(x) ((void)(x))
#define INPUT_BUFFER_SIZE 1024

volatile bool run = true;

void sigint_handler(int signum)
{
    UNUSED(signum);
    run = false;
}

int main(int argc, char **argv)
{
    telemetry_connection_t connection;
    void *param;
    const char *name = "localhost";
    int16_t port = TELEMETRY_SINK_PORT;
    char input_buffer[INPUT_BUFFER_SIZE];
    ssize_t read_amount = 0, curr_write_amount, total_written;
    int result = 0;

	UNUSED(argc);
	UNUSED(argv);

    signal(SIGINT, sigint_handler);

    /* initialize connection */
    if (!connection_init(&connection, "sink_client",
                         connection_actor, socket_writer, socket_reader))
    {
        puts("couldn't initialize");
        return 1;
    }
    connection.fd = sink_client_init();
    sprintf(connection.metadata, "%s:%d", name, port);

    /* connect */
    param = get_client_param(name, port);
    if (!telemetry_connection_connect(&connection, param))
    {
        puts("couldn't connect");
        return 1;
    }

    while (run)
    {
        read_amount = connection_read(&connection, input_buffer,
                                      INPUT_BUFFER_SIZE);
        if (read_amount < 0)
        {
            result = 1;
            run = false;
        }
        total_written = 0;
        while (total_written < read_amount)
        {
            curr_write_amount = write(STDOUT_FILENO,
                                      &input_buffer[total_written],
                                      read_amount - total_written);
            if (curr_write_amount < 0)
            {
                puts("write to stdout failed");
                result = 1;
                run = false;
                break;
            }
            total_written += curr_write_amount;
        }
    }

    if (connection.state == TELEMETRY_CONNECTION_CONNECTED)
        telemetry_connection_disconnect(&connection, NULL);

	return result;
}
