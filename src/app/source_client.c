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

	UNUSED(argc);
	UNUSED(argv);

    signal(SIGINT, sigint_handler);

    /* initialize connection */
    if (!connection_init(&connection, "sink client",
                         connection_actor, socket_writer, socket_reader))
    {
        puts("couldn't initialize");
        return 1;
    }

    /* connect */
    if (!telemetry_connection_connect(&connection, NULL)) // TODO
    {
        puts("couldn't connect");
        return 1;
    }

    while (run)
    {
        /* poll stdin? */
    }

    telemetry_connection_disconnect(&connection, NULL);

	return 0;
}
