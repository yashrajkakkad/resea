#include <virtio/virtio.h>

io_t common_cfg_io;
offset_t common_cfg_off;
io_t device_cfg_io;
offset_t device_cfg_off;
io_t notify_struct_io;
offset_t notify_cap_off;
offset_t notify_off_multiplier;
io_t isr_struct_io;
offset_t isr_cap_off;

uint8_t read_device_status(void) {
    return VIRTIO_COMMON_CFG_READ8(device_status);
}

void write_device_status(uint8_t value) {
    VIRTIO_COMMON_CFG_WRITE8(device_status, value);
}

void write_driver_feature(uint64_t value) {
    // Select and set feature bits 0 to 31.
    VIRTIO_COMMON_CFG_WRITE32(driver_feature_select, 0);
    VIRTIO_COMMON_CFG_WRITE32(driver_feature, value & 0xffffffff);

    // Select and set feature bits 32 to 63.
    VIRTIO_COMMON_CFG_WRITE32(driver_feature_select, 1);
    VIRTIO_COMMON_CFG_WRITE32(driver_feature, value >> 32);
}

uint16_t read_num_virtq(void) {
    return VIRTIO_COMMON_CFG_READ16(num_queues);
}

void virtq_select(unsigned index) {
    VIRTIO_COMMON_CFG_WRITE8(queue_select, index);
}

uint16_t virtq_size(void) {
    VIRTIO_COMMON_CFG_WRITE16(queue_size, 4); // FIXME:
    return VIRTIO_COMMON_CFG_READ16(queue_size);
}

void virtq_enable(void) {
    VIRTIO_COMMON_CFG_WRITE16(queue_enable, 1);
}

void virtq_set_desc_paddr(uint64_t paddr) {
    VIRTIO_COMMON_CFG_WRITE32(queue_desc_lo, paddr & 0xffffffff);
    VIRTIO_COMMON_CFG_WRITE32(queue_desc_hi, paddr >> 32);
}

void virtq_set_driver_paddr(uint64_t paddr) {
    VIRTIO_COMMON_CFG_WRITE32(queue_driver_lo, paddr & 0xffffffff);
    VIRTIO_COMMON_CFG_WRITE32(queue_driver_hi, paddr >> 32);
}

void virtq_set_device_paddr(uint64_t paddr) {
    VIRTIO_COMMON_CFG_WRITE32(queue_device_lo, paddr & 0xffffffff);
    VIRTIO_COMMON_CFG_WRITE32(queue_device_hi, paddr >> 32);
}

void virtq_notify(struct virtio_virtq *vq) {
    io_write16(notify_struct_io, vq->queue_notify_off, vq->index);
}

bool virtq_is_desc_free(struct virtio_virtq *vq, struct virtq_desc *desc) {
    int avail = !!(desc->flags & VIRTQ_DESC_F_AVAIL);
    int used = !!(desc->flags & VIRTQ_DESC_F_USED);
    return avail == used;
}

bool virtq_is_desc_used(struct virtio_virtq *vq, struct virtq_desc *desc) {
    int avail = !!(desc->flags & VIRTQ_DESC_F_AVAIL);
    int used = !!(desc->flags & VIRTQ_DESC_F_USED);
    return avail == used && used == vq->used_wrap_counter;
}

int virtq_alloc(struct virtio_virtq *vq, size_t len) {
    int index = vq->next_avail;
    struct virtq_desc *desc = &vq->descs[index];

    if (!virtq_is_desc_free(vq, desc)) {
        // The desciptor is not free.
        return -1;
    }

    desc->flags =
        (vq->avail_wrap_counter << VIRTQ_DESC_F_AVAIL_SHIFT)
        | (!vq->avail_wrap_counter << VIRTQ_DESC_F_USED_SHIFT);
    desc->len = len;
    desc->id = index;

    vq->next_avail++;
    if (vq->next_avail == vq->num_descs) {
        vq->avail_wrap_counter ^= 1;
        vq->next_avail = 0;
    }

    return index;
}

struct virtq_desc *virtq_pop_used(struct virtio_virtq *vq) {
    struct virtq_desc *desc = &vq->descs[vq->next_used];
    INFO("pop index = %d: flags=%x, wrap=%d [%s]", vq->next_used, desc->flags, vq->used_wrap_counter,
        !virtq_is_desc_used(vq, desc) ? "empty" : "full");
    if (!virtq_is_desc_used(vq, desc)) {
        return NULL;
    }

    DBG("pop index = %d: max=%d, %d", vq->next_used, vq->num_descs, vq->next_used == vq->num_descs - 1);
    return desc;
}

void virtq_free_used(struct virtio_virtq *vq, struct virtq_desc *desc) {
    desc->flags =
        (!vq->used_wrap_counter << VIRTQ_DESC_F_AVAIL_SHIFT)
        | (vq->used_wrap_counter << VIRTQ_DESC_F_USED_SHIFT);

    vq->next_used++;
    if (vq->next_used == vq->num_descs) {
        vq->used_wrap_counter ^= 1;
        vq->next_used = 0;
    }
}

/// Reads the ISR status and de-assert an interrupt
/// ("4.1.4.5 ISR status capability").
uint8_t read_isr_status(void) {
    return io_read8(isr_struct_io, isr_cap_off);
}
