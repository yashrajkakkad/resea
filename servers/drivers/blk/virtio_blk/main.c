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

static struct virtio_blk_req_buffer *virtq_blk_req_buffer(struct virtio_virtq *vq,
                                                  unsigned index) {
    return &((struct virtio_blk_req_buffer *) vq->buffers)[index];
}

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

void main(void) {
    TRACE("starting...");

    // Look for and initialize a virtio-net device.
    uint8_t irq;
    ASSERT_OK(virtio_find_device(2, &virtio, &irq));

    // Negotiate required features
    virtio->negotiate_feature(VIRTIO_BLK_F_SEG_MAX | VIRTIO_BLK_F_BLK_SIZE | VIRTIO_BLK_F_GEOMETRY);

    virtio->virtq_init(0);
    req_virtq = virtio->virtq_get(0);
    virtio->virtq_allocate_buffers(req_virtq, sizeof(struct virtio_blk_req_buffer), false);
    // virtio->virtq_allocate_buffers(req_virtq, 1, sizeof(struct virtio_blk_req_header), false);
    // virtio->virtq_allocate_buffers(req_virtq, 1, sizeof(struct virtio_blk_req_buffer), false);
    // virtio->virtq_allocate_buffers(req_virtq, 1, sizeof(uint8_t), true);

    // Start listening for interrupts.
    ASSERT_OK(irq_acquire(irq));

    // Make the device active.
    virtio->activate();

    int packet1 = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_header));
    struct virtio_blk_req_header *blk_header = &((struct virtio_blk_req_header *) req_virtq->buffers)[packet1];
    blk_header->type = VIRTIO_BLK_T_OUT;
    blk_header->reserved = 0;
    blk_header->sector = 1;
    req_virtq->modern.descs[packet1].flags |= VIRTQ_DESC_F_NEXT;

    int packet2 = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_buffer));
    struct virtio_blk_req_buffer *blk_buffer = &((struct virtio_blk_req_buffer *) req_virtq->buffers)[packet2];
    blk_buffer->data[0] = 69;
    req_virtq->modern.descs[packet2].flags |= VIRTQ_DESC_F_NEXT;

    int packet3 = virtio->virtq_alloc(req_virtq, sizeof(uint8_t));
    req_virtq->modern.descs[packet3].flags |= VIRTQ_DESC_F_WRITE;
    req_virtq->modern.descs[packet3].id = into_le16(0);

    virtio->virtq_kick_desc(req_virtq, packet1);

    DBG("Finished writing...");

    int packet4 = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_header));
    struct virtio_blk_req_header *blk_header2 = &((struct virtio_blk_req_header *) req_virtq->buffers)[packet4];
    blk_header2->type = VIRTIO_BLK_T_IN;
    blk_header2->reserved = 0;
    blk_header2->sector = 1;
    req_virtq->modern.descs[packet4].flags |= VIRTQ_DESC_F_NEXT;

    int packet5 = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_buffer));
    req_virtq->modern.descs[packet5].flags |= VIRTQ_DESC_F_NEXT;

    int packet6 = virtio->virtq_alloc(req_virtq, sizeof(uint8_t));
    req_virtq->modern.descs[packet6].flags |= VIRTQ_DESC_F_WRITE;
    req_virtq->modern.descs[packet6].id = into_le16(1);

    virtio->virtq_kick_desc(req_virtq, packet4);

    struct virtio_blk_req_buffer *recv_data = &((struct virtio_blk_req_buffer *) req_virtq->buffers)[packet5];
    DBG("%d", recv_data->data[0]);

    // uint8_t *status = &((uint8_t *)req_virtq->buffers)[packet3];
    // DBG("Status = %d", status);

    // offset_t base = offsetof(struct virtio_blk_config, capacity_lo);
    // uint32_t cap_lo = virtio->read_device_config(base, sizeof(uint32_t));
    // base = offsetof(struct virtio_blk_config, capacity_hi);
    // uint32_t cap_hi = virtio->read_device_config(base, sizeof(uint32_t));
    // DBG("Capacity = %d", ((((uint64_t) cap_hi) << 32) | cap_lo));
    // base = offsetof(struct virtio_blk_config, seg_max);
    // DBG("Max. number of segments in a request = %d", virtio->read_device_config(base, sizeof(uint32_t)));
    // base = offsetof(struct virtio_blk_config, blk_size);
    // DBG("Block size of disk = %d", virtio->read_device_config(base, sizeof(uint32_t)));
}
