#include "io.h"

#include <stdio.h>

#define UNUSED(x) ((void)(x))

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
    int16_t port = TELEMETRY_SOURCE_PORT;
    int result = 0, read_result;
    char curr;

	UNUSED(argc);
	UNUSED(argv);

    signal(SIGINT, sigint_handler);

    /* initialize connection */
    if (!connection_init(&connection, "source_client",
                         connection_actor, socket_writer, socket_reader))
    {
        puts("couldn't initialize");
        return 1;
    }
    connection.fd = source_client_init();
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
        read_result = read(STDIN_FILENO, &curr, 1);
        if (read_result != 1)
        {
            result = 1;
            run = false;
        }
        else if (connection_write(&connection, &curr, 1) != 1)
        {
            puts("write to connection failed");
            result = 1;
            run = false;
        }
    }

    telemetry_connection_disconnect(&connection, NULL);

	return result;
}
