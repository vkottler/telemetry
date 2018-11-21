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
    telemetry_server_t server;

	UNUSED(argc);
	UNUSED(argv);

    signal(SIGINT, sigint_handler);

    if (!telemetry_server_init(&server, sink_server_init, source_server_init, 
                               sink_connection_manager,
                               source_connection_manager, socket_closer))
    {
        fprintf(stderr, "%s: couldn't initialize server\r\n", __func__);
        return 1;
    }

    while (run)
    {
        telemetry_server_service_all(&server);
    }

    telemetry_server_stop(&server);

	return 0;
}
