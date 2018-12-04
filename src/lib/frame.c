/*
 * Vaughn Kottler, 12/02/2018
 */

#include <unistd.h>
#include <stdlib.h>

#include "frame.h"

/*
 * Read the next frame from a file descriptor.
 */
frame_t *frame_read(int fd)
{
    char         curr = '\0';
    frame_t     *frame;
    frame_type_t frame_type;
    size_t       frame_size, total_read;
    ssize_t      read_result;
    const char *error_msg = "read failed";

    /* await start-of-frame */
    while (curr != TELEM_SOF)
    {
        if (read(fd, &curr, 1) != 1)
        {
            error_msg = "read failed: SOF";
            goto io_failed;
        }
    }

    /* read frame type */
    if (read(fd, &curr, 1) != 1)
    {
        error_msg = "read failed: type";
        goto io_failed;
    }
    frame_type = (frame_type_t) curr;

    /* read size of frame */
    if (read(fd, &curr, 1) != 1)
    {
        error_msg = "read failed: size";
        goto io_failed;
    }
    frame_size = (size_t) curr;

    /* allocate space for the frame */
    frame = malloc(frame_size + sizeof(frame_t) - sizeof(void *));
    if (frame == NULL)
    {
        error_msg = "malloc failed";
        goto io_failed;
    }
    frame->type = frame_type;
    frame->size = frame_size;

    /* read data */
    total_read = 0;
    while (frame_size)
    {
        read_result = read(fd, ((char *) &frame->data) + total_read, frame_size);
        if (read_result <= 0)
        {
            error_msg = "read failed: data";
            goto io_failed;
        }
        frame_size -= read_result;
        total_read += read_result;
    }

    /* await end-of-frame */
    if (read(fd, &curr, 1) != 1)
    {
        error_msg = "read failed: EOF";
        goto io_failed;
    }
    else if (curr != TELEM_EOF)
    {
        error_msg = "expected EOF, got something else";
        goto io_failed;
    }
    return frame;

io_failed:
    telemetry_debug("%s: %s\r\n", __func__, error_msg);
    return NULL;
}

/*
 * Call the appropriate handler for a given frame.
 */
int handle_frame(frame_handler_t *handler, frame_t *frame)
{
    switch (frame->type)
    {
        case TELEM_FRAME_CONSOLE:
            return handler->console((const char *) &frame->data, frame->size);
        case TELEM_FRAME_DATA:
            return handler->data((telemetry_packet_t *) &frame->data);
        case TELEM_FRAME_MANIFEST:
            return handler->manifest((const char *) &frame->data, frame->size);
        default:
            telemetry_debug("%s: unknown frame type\r\n", __func__);
            return -1;
    }
}
