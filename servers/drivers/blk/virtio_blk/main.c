#include <driver/dma.h>
#include <driver/io.h>
#include <driver/irq.h>
#include <endian.h>
#include <resea/async.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>
#include <virtio/virtio.h>
#include "virtio_blk.h"

static struct virtio_ops *virtio = NULL;
static struct virtio_virtq *req_virtq = NULL;

// static struct virtio_blk_req_buffer *virtq_blk_req_buffer(struct virtio_virtq *vq,
//                                                   unsigned index) {
//     return &((struct virtio_blk_req_buffer *) vq->buffers)[index];
// }

void driver_handle_interrupt(void) {
    uint8_t status = virtio->read_isr_status();
    if (status & 1) {
        int index;
        size_t len;
        while (virtio->virtq_pop_desc(req_virtq, &index, &len) == OK) {
            DBG("%d %d", index, len);
            virtio->virtq_push_desc(req_virtq, index);
        }

        virtio->virtq_notify(req_virtq);
    }
}

static error_t virtio_blk_read(offset_t sector, uint8_t *buf) {
    int header_desc = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_header));
    struct virtio_blk_req_header *blk_req_header = &((struct virtio_blk_req_header *) req_virtq->buffers)[header_desc];
    blk_req_header->type = VIRTIO_BLK_T_IN;
    blk_req_header->reserved = 0;
    blk_req_header->sector = sector;
    req_virtq->modern.descs[header_desc].flags |= VIRTQ_DESC_F_NEXT;

    int buffer_desc = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_buffer));
    req_virtq->modern.descs[buffer_desc].flags |= (VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE);

    int status_desc = virtio->virtq_alloc(req_virtq, sizeof(uint8_t));
    req_virtq->modern.descs[status_desc].flags |= VIRTQ_DESC_F_WRITE;
    req_virtq->modern.descs[status_desc].id = into_le16(0);

    virtio->virtq_kick_desc(req_virtq, header_desc);

    struct virtio_blk_req_buffer *recv_data = &((struct virtio_blk_req_buffer *) req_virtq->buffers)[buffer_desc];

    for (size_t i = 0; i < BUF_SIZE; i++) {
        buf[i] = recv_data->data[i];
    }

    return OK;
}

static error_t virtio_blk_write(offset_t sector, const uint8_t *buf) {
    int header_desc = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_header));
    struct virtio_blk_req_header *blk_req_header = &((struct virtio_blk_req_header *) req_virtq->buffers)[header_desc];
    blk_req_header->type = VIRTIO_BLK_T_OUT;
    blk_req_header->reserved = 0;
    blk_req_header->sector = sector;
    req_virtq->modern.descs[header_desc].flags |= VIRTQ_DESC_F_NEXT;

    int buffer_desc = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_buffer));
    struct virtio_blk_req_buffer *blk_req_buffer = &((struct virtio_blk_req_buffer *) req_virtq->buffers)[buffer_desc];
    for (size_t i = 0; i < BUF_SIZE; i++) {
        blk_req_buffer->data[i] = buf[i];
    }
    req_virtq->modern.descs[buffer_desc].flags |= (VIRTQ_DESC_F_NEXT);

    int status_desc = virtio->virtq_alloc(req_virtq, sizeof(uint8_t));
    req_virtq->modern.descs[status_desc].flags |= VIRTQ_DESC_F_WRITE;
    req_virtq->modern.descs[status_desc].id = into_le16(header_desc);

    virtio->virtq_kick_desc(req_virtq, header_desc);

    return OK;
}

void main(void) {
    TRACE("starting...");

    // Look for and initialize a virtio-net device.
    uint8_t irq;
    ASSERT_OK(virtio_find_device(2, &virtio, &irq));

    // Negotiate required features
    virtio->negotiate_feature(VIRTIO_BLK_F_SEG_MAX | VIRTIO_BLK_F_BLK_SIZE | VIRTIO_BLK_F_GEOMETRY);

    offset_t base = offsetof(struct virtio_blk_config, capacity_lo);
    uint32_t cap_lo = virtio->read_device_config(base, sizeof(uint32_t));
    base = offsetof(struct virtio_blk_config, capacity_hi);
    uint32_t cap_hi = virtio->read_device_config(base, sizeof(uint32_t));
    DBG("Capacity = %d", ((((uint64_t) cap_hi) << 32) | cap_lo));
    base = offsetof(struct virtio_blk_config, seg_max);
    DBG("Max. number of segments in a request = %d", virtio->read_device_config(base, sizeof(uint32_t)));
    base = offsetof(struct virtio_blk_config, blk_size);
    DBG("Block size of disk = %d", virtio->read_device_config(base, sizeof(uint32_t)));

    virtio->virtq_init(0);
    req_virtq = virtio->virtq_get(0);
    virtio->virtq_allocate_buffers(req_virtq, sizeof(struct virtio_blk_req_buffer), false);

    // Start listening for interrupts.
    ASSERT_OK(irq_acquire(irq));

    // Make the device active.
    virtio->activate();

    // DBG("Writing...");
    // virtio_blk_write(1);
    // DBG("Reading...");
    // virtio_blk_read(1);

    ASSERT_OK(ipc_serve("disk"));
    TRACE("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        // TODO: get the disk size
        switch (m.type) {
            case BLK_READ_MSG: {
                size_t sector = m.blk_read.sector;
                size_t len = m.blk_read.num_sectors * SECTOR_SIZE;
                uint8_t buf[BUF_SIZE];
                error_t err = virtio_blk_read(sector, buf);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }
                m.type = BLK_READ_REPLY_MSG;
                m.blk_read_reply.data = buf;
                m.blk_read_reply.data_len = len;
                ipc_reply(m.src, &m);
                break;
            }
            case BLK_WRITE_MSG: {
                error_t err = virtio_blk_write(m.blk_write.sector, m.blk_write.data);
                free(m.blk_write.data);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                m.type = BLK_WRITE_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
