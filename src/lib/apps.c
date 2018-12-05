/*
 * Vaughn Kottler, 12/04/2018
 */

#include "apps.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

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

int get_socket(void)
{
    return socket(AF_INET, SOCK_STREAM, 0);
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
