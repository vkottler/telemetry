/*
 * Vaughn Kottler, 11/19/2018
 */

#pragma once

#include "manifest.h"

/*
 * Retrieve the manifest indices segment from the blob.
 */
#define PACKET_INDICES(packet)  ((uint32_t *) &packet->blob)

/*
 * Retrieve the data segment of the packet from the blob.
 */
#define PACKET_DATA(packet) ((void *) &((uint32_t *) &packet->blob)[packet->channel_count])

typedef struct __attribute__((__packed__)) _telemetry_packet {
    uint32_t  channel_count;
    size_t    data_size;
    uint32_t  crc32;
    void     *blob;
} telemetry_packet_t;

size_t telemetry_packet_compute_data_size(channel_t *channels, uint32_t count);
size_t telemetry_packet_compute_size(channel_t *channels, uint32_t count);
size_t telemetry_packet_size(telemetry_packet_t *packet);

telemetry_packet_t *telemetry_packet_create(channel_t *channels,
                                            uint32_t count);
void telemetry_packet_initialize(telemetry_packet_t *packet,
                                 channel_t *channels,
                                 uint32_t count);
telemetry_packet_t **telemetry_packets_from_manifest(channel_manifest_t *manifest,
                                                     size_t mtu, size_t *npackets);

void telemetry_packet_print(FILE *stream, telemetry_packet_t *packet);
