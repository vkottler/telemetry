/*
 * Vaughn Kottler, 12/04/2018
 */

#include "apps.h"
#include "telemetry.h"

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

volatile int run = 1;
void sigint_handler(int signum)
{
    UNUSED(signum);
    run = 0;
}

void init_manifest(channel_manifest_t *manifest)
{
    channel_add(manifest, "gyro_x", "deg/s", TELEM_INT16, sizeof(int16_t));
    channel_add(manifest, "gyro_y", "deg/s", TELEM_INT16, sizeof(int16_t));
    channel_add(manifest, "gyro_z", "deg/s", TELEM_INT16, sizeof(int16_t));

    channel_add(manifest, "lidar_d1", "mm", TELEM_UINT16, sizeof(uint16_t));
    channel_add(manifest, "lidar_d2", "mm", TELEM_UINT16, sizeof(uint16_t));

    channel_add(manifest, "batt_v_cell1", "V", TELEM_FLOAT, sizeof(float));
    channel_add(manifest, "batt_v_cell2", "V", TELEM_FLOAT, sizeof(float));
    channel_add(manifest, "batt_v_cell3", "V", TELEM_FLOAT, sizeof(float));
    channel_add(manifest, "batt_current", "V", TELEM_FLOAT, sizeof(float));
    channel_add(manifest, "batt_v_total", "V", TELEM_FLOAT, sizeof(float));
}

int main(int argc, char **argv)
{
    struct timespec sleep_duration;
    channel_manifest_t *manifest;
    int console_fd = get_socket();
    int data_fd = get_socket();
    int manifest_fd = get_socket();
    sleep_duration.tv_nsec = 0;
    sleep_duration.tv_sec = 1;
    UNUSED(argc);
    UNUSED(argv);

    /* open client connections */
    if (client_connect(console_fd, CONSOLE_HOST, CONSOLE_PORT) ||
        client_connect(data_fd, DATA_HOST, DATA_PORT) ||
        client_connect(manifest_fd, MANIFEST_HOST, MANIFEST_PORT))
    {
        fprintf(stderr, "couldn't establish a client connection");
        return 1;
    }

    /* send a manifest */
    manifest = channel_manifest_create(255);
    init_manifest(manifest);

    /* send spoofed telemetry */
    signal(SIGINT, sigint_handler);
    while (run)
    {
        if (write(console_fd, PING_STR, strlen(PING_STR)) == -1)
        {
            fprintf(stderr, "write to console_fd failed");
            run = 0;
        }
        nanosleep(&sleep_duration, NULL);
    }

    /* close client connections */
    close_socket(console_fd);
    close_socket(data_fd);
    close_socket(manifest_fd);

    return 0;
}
