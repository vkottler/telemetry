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

int console_fd = -1;
int data_fd = -1;
int manifest_fd = -1;

volatile int run = 1;
void sigint_handler(int signum)
{
    UNUSED(signum);
    run = 0;
}

void manifest_sender(char *buf, uint32_t len)
{
    ssize_t result = write(manifest_fd, buf, len);
    if (result < 0 ||  (uint32_t) result != len)
    {
        fprintf(stderr, "error sending manifest\r\n");
        run = 0;
    }
}

void update_telemetry(channel_manifest_t *manifest)
{
    *((int16_t *) manifest->channels[0].data) += 1;
    *((int16_t *) manifest->channels[1].data) += 1;
    *((int16_t *) manifest->channels[2].data) += 1;

    *((uint16_t *) manifest->channels[3].data) += 1;
    *((uint16_t *) manifest->channels[4].data) += 1;

    *((float *) manifest->channels[5].data) += 1.0f;
    *((float *) manifest->channels[6].data) += 1.0f;
    *((float *) manifest->channels[7].data) += 1.0f;
    *((float *) manifest->channels[8].data) += 1.0f;
    *((float *) manifest->channels[9].data) += 1.0f;
}

void send_telemetry(telemetry_packet_t **packets, uint32_t num_packets)
{
    uint32_t i;
    uint32_t packet_len;
    ssize_t result;
    for (i = 0; i < num_packets; i++)
    {
        packet_len = telemetry_packet_size(packets[i]);
        result = write(data_fd, packets[i], packet_len);
        if (result < 0 ||  (uint32_t) result != packet_len)
        {
            fprintf(stderr, "error sending telemetry\r\n");
            run = 0;
            break;
        }
    }
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
    telemetry_packet_t **packets;
    uint32_t num_packets;
    sleep_duration.tv_nsec = 250000000;
    sleep_duration.tv_sec = 0;
    UNUSED(argc);
    UNUSED(argv);

    /* open client connections */
    console_fd = get_socket();
    data_fd = get_socket();
    manifest_fd = get_socket();
    if (client_connect(console_fd, CONSOLE_HOST, CONSOLE_PORT) ||
        client_connect(data_fd, DATA_HOST, DATA_PORT) ||
        client_connect(manifest_fd, MANIFEST_HOST, MANIFEST_PORT))
    {
        fprintf(stderr, "couldn't establish a client connection\r\n");
        return 1;
    }

    /* send a manifest */
    manifest = channel_manifest_create(255);
    init_manifest(manifest);
    channel_manifest_send(manifest, manifest_sender);
    close_socket(manifest_fd);

    /* initialize packets */
    packets = telemetry_packets_from_manifest(manifest, 60, &num_packets);

    /* send spoofed telemetry */
    signal(SIGINT, sigint_handler);
    while (run)
    {
        if (write(console_fd, PING_STR, strlen(PING_STR)) == -1)
        {
            fprintf(stderr, "write to console_fd failed");
            run = 0;
        }
        update_telemetry(manifest);
        send_telemetry(packets, num_packets);
        nanosleep(&sleep_duration, NULL);
    }

    /* close client connections */
    close_socket(console_fd);
    close_socket(data_fd);

    return 0;
}
