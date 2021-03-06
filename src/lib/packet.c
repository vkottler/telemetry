/*
 * Vaughn Kottler, 10/18/2018
 */

#include <stdlib.h>
#include <string.h>

#include "telemetry.h"

/*
 * From an array of channels, compute the size of a telemetry packet data
 * blob.
 */
uint32_t telemetry_packet_compute_data_size(channel_t *channels, uint8_t count)
{
    uint32_t i;
    uint32_t data_size = 0;
    for (i = 0; i < count; i++)
    {
        data_size += channels[i].size;
    }
    return data_size;
}

/*
 * From an array of channels, compute the overall size of a telemetry packet.
 */
uint32_t telemetry_packet_compute_size(channel_t *channels, uint8_t count)
{
    uint32_t packet_size;
    uint32_t data_size = telemetry_packet_compute_data_size(channels, count);

    /* calculate overall size of packet */
    packet_size  = sizeof(telemetry_packet_t) - sizeof(void *);
    packet_size += count * sizeof(uint8_t);
    packet_size += data_size;

    return packet_size;
}

/*
 * Build a telemetry packet from the given channels.
 *
 * Sets each channel's data pointer to its correct position in the blob.
 */
telemetry_packet_t *telemetry_packet_create(channel_t *channels,
                                            uint8_t count)
{
    uint32_t packet_size;

    /* calculate overall size of packet */
    packet_size = telemetry_packet_compute_size(channels, count);
    packet_size += telemetry_packet_compute_data_size(channels, count);

    /* construct packet */
    telemetry_packet_t *packet = malloc(packet_size);
    if (!packet)
    {
        telemetry_debug("%s: malloc failed\r\n", __func__);
        return NULL;
    }
    telemetry_packet_initialize(packet, channels, count);
    return packet;
}

/*
 * Initialize a newly created or empty telemetry packet with default values
 * and format it for transport.
 */
void telemetry_packet_initialize(telemetry_packet_t *packet,
                                 channel_t *channels,
                                 uint8_t count)
{
    uint32_t i;
    uint8_t *indices;
    uint8_t *data;

    packet->channel_count = count;
    packet->data_size = telemetry_packet_compute_data_size(channels, count);

    /* initialize manifest indices */
    indices = PACKET_INDICES(packet);
    for (i = 0; i < count; i++)
        indices[i] = channels[i].manifest_index;

    /* initialize all data to zero */
    memset(PACKET_DATA(packet), 0, packet->data_size);

    /* set channels' data pointers */
    data = (uint8_t *) PACKET_DATA(packet);
    for (i = 0; i < count; i++)
    {
        channels[i].data = data;
        data += channels[i].size;
    }
}

/*
 * Create an array of size-optimized telemetry packets from a channel manifest.
 */
telemetry_packet_t **telemetry_packets_from_manifest(channel_manifest_t *manifest,
                                                    uint32_t mtu, uint32_t *npackets)
{
    telemetry_packet_t **telemetry_packets;
    telemetry_packet_t **pbuffer;
    uint32_t curr, max_packet_size, capacity, working_size;
    uint8_t channel_idx, channel_count;
    uint32_t i;

    /* initialize working data */
    capacity = TELEMETRY_CAPACITY;
    working_size = 0;
    channel_idx = 0;
    channel_count = 0;
    *npackets = 0;
    pbuffer = malloc(capacity * sizeof(telemetry_packet_t *));
    if (!pbuffer)
    {
        telemetry_debug("%s: malloc failed\r\n", __func__);
        return NULL;
    }

    /* get total size of all channel data */
    max_packet_size = mtu - (sizeof(telemetry_packet_t) - sizeof(void *));
    for (i = 0; i < manifest->count; i++)
    {
        curr = manifest->channels[i].size;

        /* if adding this channel would overflow, make a new packet */
        if (working_size + (curr + sizeof(uint8_t)) >= max_packet_size)
        {
            if (channel_count == 0 || working_size == 0)
            {
                telemetry_debug("%s: channel[%d] larger than max packet size (%u)\r\n",
                                __func__, i, max_packet_size);
                return NULL;
            }

            /* create packet */
            pbuffer[(*npackets)++] = telemetry_packet_create(&manifest->channels[channel_idx], 
                                                             channel_count);
            if (*npackets == capacity)
            {
                pbuffer = realloc(pbuffer, capacity * 2 * sizeof(telemetry_packet_t *));
                if (!pbuffer)
                {
                    telemetry_debug("%s: malloc failed\r\n", __func__);
                    return NULL;
                }
                capacity = capacity * 2;
            }

            /* get ready for next round */
            channel_idx = i;
            working_size = 0;
            channel_count = 0;
        }
        working_size += curr + sizeof(uint8_t);
        channel_count++;
    }
    
    /* add the last packet */
    pbuffer[(*npackets)++] = telemetry_packet_create(&manifest->channels[channel_idx], 
                                                     channel_count);

    /* build exact-size packet array */
    telemetry_packets = malloc(*npackets * sizeof(telemetry_packet_t *));
    if (!telemetry_packets)
    {
        telemetry_debug("%s: malloc failed\r\n", __func__);
        return NULL;
    }
    for (i = 0; i < *npackets; i++)
        telemetry_packets[i] = pbuffer[i];
    free(pbuffer);

    return telemetry_packets;
}

/*
 * Get the total size of a telemetry packet.
 */
uint32_t telemetry_packet_size(telemetry_packet_t *packet)
{
    uint32_t result = sizeof(telemetry_packet_t) - sizeof(void *);
    result += packet->channel_count * sizeof(uint8_t);
    result += packet->data_size;
    return result;
}

/*
 * Print a telemetry packet to the given IO stream.
 */
void telemetry_packet_print(FILE *stream, telemetry_packet_t *packet)
{
    fputs("--------------------\r\n", stream);
    fprintf(stream, "Channels:   %u\r\n", packet->channel_count);
    fputs("Indices:    ", stream);
    uint8_t *indices = PACKET_INDICES(packet);
    for (uint8_t i = 0; i < packet->channel_count; i++)
    {
        if (i == packet->channel_count - 1)
            fprintf(stream, "%u\r\n", indices[i]);
        else
            fprintf(stream, "%u, ", indices[i]);
    }
    fprintf(stream, "Data  Size: %u\r\n", packet->data_size);
    fprintf(stream, "Total Size: %u\r\n", telemetry_packet_size(packet));
    fputs("--------------------\r\n", stream);
}
