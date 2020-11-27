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
    // for (int i = 0; i < req_virtq->num_descs; i++) {
    //     struct virtio_blk_req_buffer *buf = virtq_blk_req_buffer(req_virtq, i);
    // }

    // Start listening for interrupts.
    ASSERT_OK(irq_acquire(irq));

    // Make the device active.
    virtio->activate();

    offset_t base = offsetof(struct virtio_blk_config, capacity_lo);
    uint32_t cap_lo = virtio->read_device_config(base, sizeof(uint32_t));
    base = offsetof(struct virtio_blk_config, capacity_hi);
    uint32_t cap_hi = virtio->read_device_config(base, sizeof(uint32_t));
    DBG("Capacity = %d", ((((uint64_t) cap_hi) << 32) | cap_lo));
    base = offsetof(struct virtio_blk_config, seg_max);
    DBG("Max. number of segments in a request = %d", virtio->read_device_config(base, sizeof(uint32_t)));
    base = offsetof(struct virtio_blk_config, blk_size);
    DBG("Block size of disk = %d", virtio->read_device_config(base, sizeof(uint32_t)));

    int index = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_buffer));
    struct virtio_blk_req_buffer *buf = virtq_blk_req_buffer(req_virtq, index);
    buf->header.type = VIRTIO_BLK_T_OUT;
    buf->header.sector = 512;
    buf->header.reserved = 0;
    for (size_t i = 0; i < 512; i++) {
        buf->data[i] = 1;
    }

    virtio->virtq_kick_desc(req_virtq, index);

    // int index1 = virtio->virtq_alloc(req_virtq, sizeof(struct virtio_blk_req_header));
    // struct virtio_blk_req_header *buf1 = &((struct virtio_blk_req_header *) req_virtq->buffers)[index1];
    // buf1->type = 1;
    // buf1->reserved = 0;
    // buf1->sector = 512;

    // int index2 = virtio->virtq_alloc(req_virtq, 512);
    // struct virtio_blk_req_buffer *buf2 = &((struct virtio_blk_req_buffer *) req_virtq->buffers)[index2];
    // for (size_t i = 0; i < 512; i++) {
    //     buf2->data[i] = 1;
    // }

    // int index3 = virtio->virtq_alloc(req_virtq, 1);
    // uint8_t *status = &((uint8_t *) req_virtq->buffers)[index3];
}
