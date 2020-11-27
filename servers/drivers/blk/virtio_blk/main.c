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
}
