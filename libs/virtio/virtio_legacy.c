#include <endian.h>
#include <driver/io.h>
#include <virtio/virtio.h>

/// The maximum number of virtqueues.
#define NUM_VIRTQS_MAX 8

static task_t dm_server;
static struct virtio_virtq virtqs[NUM_VIRTQS_MAX];
static offset_t notify_off_multiplier;
io_t common_cfg_io = NULL;

static uint8_t read_device_status(void) {
    return VIRTIO_COMMON_CFG_READ8(device_status);
}

static void write_device_status(uint8_t value) {
    VIRTIO_COMMON_CFG_WRITE8(device_status, value);
}

static uint64_t read_device_features(void) {
    // Select and read feature bits 0 to 31.
    VIRTIO_COMMON_CFG_WRITE32(device_feature_select, 0);
    uint32_t feats_lo =
        VIRTIO_COMMON_CFG_READ32(device_feature);

    // Select and read feature bits 32 to 63.
    VIRTIO_COMMON_CFG_WRITE32(device_feature_select, 1);
    uint32_t feats_hi =
        VIRTIO_COMMON_CFG_READ32(device_feature);

    return (((uint64_t) feats_hi) << 32) | feats_lo;
}

/// Returns the number of virtqueues in the device.
static uint16_t num_virtqueues(void) {
    return VIRTIO_COMMON_CFG_READ16(num_queues);
}

/// Returns true if the descriptor is available for the output to the device.
/// XXX: should we count used_wrap_counter and use is_desc_used() instead?
static bool is_desc_free(struct virtio_virtq *vq, struct virtq_desc *desc) {
    uint16_t flags = from_le16(desc->flags);
    int avail = !!(flags & VIRTQ_DESC_F_AVAIL);
    int used = !!(flags & VIRTQ_DESC_F_USED);
    return avail == used;
}

/// Returns true if the descriptor has been used by the device.
static bool is_desc_used(struct virtio_virtq *vq, struct virtq_desc *desc) {
    uint16_t flags = from_le16(desc->flags);
    int avail = !!(flags & VIRTQ_DESC_F_AVAIL);
    int used = !!(flags & VIRTQ_DESC_F_USED);
    return avail == used && used == vq->used_wrap_counter;
}

/// Selects the current virtqueue in the common config.
static void virtq_legacy_select(unsigned index) {
    VIRTIO_COMMON_CFG_WRITE8(queue_select, index);
}

/// Initializes a virtqueue.
static void virtq_legacy_init(unsigned index) {
    virtq_select(index);

    size_t num_descs = virtq_size();
    ASSERT(num_descs < 1024 && "too large queue size");

    offset_t queue_notify_off =
        notify_cap_off + VIRTIO_COMMON_CFG_READ16(queue_notify_off) * notify_off_multiplier;

    // Allocate the descriptor area.
    size_t descs_size = num_descs * sizeof(struct virtq_desc);
    dma_t descs_dma =
        dma_alloc(descs_size, DMA_ALLOC_TO_DEVICE | DMA_ALLOC_FROM_DEVICE);
    memset(dma_buf(descs_dma), 0, descs_size);

    // Allocate the driver area.
    dma_t driver_dma =
        dma_alloc(sizeof(struct virtq_event_suppress), DMA_ALLOC_TO_DEVICE);
    memset(dma_buf(driver_dma), 0, sizeof(struct virtq_event_suppress));

    // Allocate the device area.
    dma_t device_dma =
        dma_alloc(sizeof(struct virtq_event_suppress), DMA_ALLOC_TO_DEVICE);
    memset(dma_buf(device_dma), 0, sizeof(struct virtq_event_suppress));

    // Register physical addresses.
    set_desc_paddr(dma_daddr(descs_dma));
    set_driver_paddr(dma_daddr(driver_dma));
    set_device_paddr(dma_daddr(device_dma));
    VIRTIO_COMMON_CFG_WRITE16(queue_enable, 1);

    virtqs[index].index = index;
    virtqs[index].descs_dma = descs_dma;
    virtqs[index].descs = (struct virtq_desc *) dma_buf(descs_dma);
    virtqs[index].num_descs = num_descs;
    virtqs[index].queue_notify_off = queue_notify_off;
    virtqs[index].next_avail = 0;
    virtqs[index].next_used = 0;
    virtqs[index].avail_wrap_counter = 1;
    virtqs[index].used_wrap_counter = 1;
}

void virtio_legacy_activate(void) {
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER_OK);
}

/// Reads the ISR status and de-assert an interrupt
/// ("4.1.4.5 ISR status capability").
uint8_t virtio_legacy_read_isr_status(void) {
    return io_read8(isr_struct_io, isr_cap_off);
}

/// Returns the number of descriptors in total in the queue.
uint16_t virtq_legacy_size(void) {
    return VIRTIO_COMMON_CFG_READ16(queue_size);
}

/// Returns the `index`-th virtqueue.
struct virtio_virtq *virtq_legacy_get(unsigned index) {
    return &virtqs[index];
}

/// Notifies the device that the queue contains a descriptor it needs to process.
void virtq_legacy_notify(struct virtio_virtq *vq) {
    io_write16(notify_struct_io, vq->queue_notify_off, vq->index);
}

/// Allocates a descriptor for the ouput to the device (e.g. TX virtqueue in
/// virtio-net).
int virtq_legacy_alloc(struct virtio_virtq *vq, size_t len) {
    int index = vq->next_avail;
    struct virtq_desc *desc = &vq->descs[index];

    if (!is_desc_free(vq, desc)) {
        return -1;
    }

    uint16_t flags =
        (vq->avail_wrap_counter << VIRTQ_DESC_F_AVAIL_SHIFT)
        | (!vq->avail_wrap_counter << VIRTQ_DESC_F_USED_SHIFT);

    desc->flags = into_le16(flags);
    desc->len = into_le32(len);
    desc->id = into_le16(index);

    vq->next_avail++;
    if (vq->next_avail == vq->num_descs) {
        vq->avail_wrap_counter ^= 1;
        vq->next_avail = 0;
    }

    return index;
}

/// Returns the next descriptor which is already used by the device. It returns
/// NULL if no descriptors are used. If the buffer is input from the device,
/// call `virtq_push_desc` once you've handled the input.
struct virtq_desc *virtq_legacy_pop_desc(struct virtio_virtq *vq) {
    struct virtq_desc *desc = &vq->descs[vq->next_used];
    if (!is_desc_used(vq, desc)) {
        return NULL;
    }

    return desc;
}

/// Makes the descriptor available for input from the device.
void virtq_legacy_push_desc(struct virtio_virtq *vq, struct virtq_desc *desc) {
    uint16_t flags = VIRTQ_DESC_F_WRITE
        | (!vq->used_wrap_counter << VIRTQ_DESC_F_AVAIL_SHIFT)
        | (vq->used_wrap_counter << VIRTQ_DESC_F_USED_SHIFT);

    desc->len = into_le32(vq->buffer_size);
    desc->flags = into_le16(flags);

    vq->next_used++;
    if (vq->next_used == vq->num_descs) {
        vq->used_wrap_counter ^= 1;
        vq->next_used = 0;
    }
}

/// Allocates queue buffers. If `writable` is true, the buffers are initialized
/// as input ones from the device (e.g. RX virqueue in virtio-net).
void virtq_legacy_allocate_buffers(struct virtio_virtq *vq, size_t buffer_size,
                                   bool writable) {
    dma_t dma = dma_alloc(buffer_size * vq->num_descs, DMA_ALLOC_FROM_DEVICE);
    vq->buffers_dma = dma;
    vq->buffers = dma_buf(dma);
    vq->buffer_size = buffer_size;

    uint16_t flags = writable ? (VIRTQ_DESC_F_AVAIL | VIRTQ_DESC_F_WRITE) : 0;
    for (int i = 0; i < vq->num_descs; i++) {
        vq->descs[i].id = into_le16(i);
        vq->descs[i].addr = into_le64(dma_daddr(dma) + (buffer_size * i));
        vq->descs[i].len = into_le32(buffer_size);
        vq->descs[i].flags = into_le16(flags);
    }
}

/// Checks and enables features. It aborts if any of the features is not supported.
void virtio_legacy_negotiate_feature(uint64_t features) {
    features |= VIRTIO_F_VERSION_1 | VIRTIO_F_RING_PACKED;

    // Abort if the device does not support features we need.
    ASSERT((read_device_features() & features) == features);

    // Select and set feature bits 0 to 31.
    VIRTIO_COMMON_CFG_WRITE32(driver_feature_select, 0);
    VIRTIO_COMMON_CFG_WRITE32(driver_feature, features & 0xffffffff);

    // Select and set feature bits 32 to 63.
    VIRTIO_COMMON_CFG_WRITE32(driver_feature_select, 1);
    VIRTIO_COMMON_CFG_WRITE32(driver_feature, features >> 32);

    write_device_status(read_device_status() | VIRTIO_STATUS_FEAT_OK);
    ASSERT((read_device_status() & VIRTIO_STATUS_FEAT_OK) != 0);
}

static uint32_t pci_config_read(handle_t device, unsigned offset, unsigned size) {
    struct message m;
    m.type = DM_PCI_READ_CONFIG_MSG;
    m.dm_pci_read_config.handle = device;
    m.dm_pci_read_config.offset = offset;
    m.dm_pci_read_config.size = size;
    ASSERT_OK(ipc_call(dm_server, &m));
    return m.dm_pci_read_config_reply.value;
}

uint64_t virtio_legacy_read_device_config(offset_t offset, size_t size) {
    switch (size) {
        case sizeof(uint8_t): return io_read8(device_cfg_io, device_cfg_off + offset);
        case sizeof(uint16_t): return io_read16(device_cfg_io, device_cfg_off + offset);
        case sizeof(uint32_t): return io_read32(device_cfg_io, device_cfg_off + offset);
        default: UNREACHABLE();
    }
}

/// Looks for and initializes a virtio device with the given device type. It
/// sets the IRQ vector to `irq` on success.
error_t virtio_legacy_pci_init(int device_type, uint8_t *irq) {
    // Search the PCI bus for a virtio device...
    dm_server = ipc_lookup("dm");
    struct message m;
    m.type = DM_ATTACH_PCI_DEVICE_MSG;
    m.dm_attach_pci_device.vendor_id = 0x1af4;
    m.dm_attach_pci_device.device_id = 0x1000;
    ASSERT_OK(ipc_call(dm_server, &m));
    handle_t pci_device = m.dm_attach_pci_device_reply.handle;

    if (!common_cfg_io) {
        WARN_DBG("missing a BAR for the device access");
        return ERR_NOT_FOUND;
    }

    *irq = pci_config_read(pci_device, 0x3c, sizeof(uint8_t));

    // "3.1.1 Driver Requirements: Device Initialization"
    write_device_status(0); // Reset the device.
    write_device_status(read_device_status() | VIRTIO_STATUS_ACK);
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER);

    return OK;
}
