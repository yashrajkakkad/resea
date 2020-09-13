#include <resea/ipc.h>
#include <resea/async.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <driver/irq.h>
#include <driver/io.h>
#include <driver/dma.h>
#include <string.h>

//
//  Driver
//
typedef void (*receive_callback_t)(const uint8_t *payload, size_t len);
struct net_driver_ops {
    error_t (*init)(receive_callback_t receive);
    error_t (*read_macaddr)(uint8_t *mac);
    void (*transmit)(const uint8_t *payload, size_t len);
    void (*handle_interrupt)(void);
};

//
//  Virtio-net
//
static io_t common_cfg_io;
static offset_t common_cfg_off;
static io_t device_cfg_io;
static offset_t device_cfg_off;
static io_t notify_struct_io;
static offset_t notify_cap_off;
static offset_t notify_off_multiplier;

#define VIRTIO_PCI_CAP_COMMON_CFG  1
#define VIRTIO_PCI_CAP_NOTIFY_CFG  2
#define VIRTIO_PCI_CAP_DEVICE_CFG  4

// "2.1 Device Status Field"
#define VIRTIO_STATUS_ACK        1
#define VIRTIO_STATUS_DRIVER     2
#define VIRTIO_STATUS_DRIVER_OK  4
#define VIRTIO_STATUS_FEAT_OK    82

#define VIRTIO_F_VERSION_1       (1ull << 32)
#define VIRTIO_F_RING_PACKED     (1ull << 34)

#define VIRTIO_NET_F_MAC         (1 << 5)
#define VIRTIO_NET_F_STATUS      (1 << 16)
#define VIRTIO_NET_QUEUE_RX  0
#define VIRTIO_NET_QUEUE_TX  1

struct virtio_net_config {
    uint8_t mac0;
    uint8_t mac1;
    uint8_t mac2;
    uint8_t mac3;
    uint8_t mac4;
    uint8_t mac5;
    uint16_t status;
    uint16_t max_virtqueue_pairs;
    uint16_t mtu;
} __packed;


#define VIRTIO_NET_HDR_GSO_NONE 0
struct virtio_net_header {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t len;
    uint16_t gso_size;
    uint16_t checksum_start;
    uint16_t checksum_offset;
    uint16_t num_buffers;
} __packed;

struct virtio_net_buffer {
    struct virtio_net_header header;
    uint8_t payload[PAGE_SIZE - sizeof(struct virtio_net_header)];
} __packed;

#define VIRTQ_DESC_F_AVAIL_SHIFT  7
#define VIRTQ_DESC_F_USED_SHIFT   15
#define VIRTQ_DESC_F_AVAIL        (1 << VIRTQ_DESC_F_AVAIL_SHIFT)
#define VIRTQ_DESC_F_USED         (1 << VIRTQ_DESC_F_USED_SHIFT)
struct virtq_desc {
    /// The physical buffer address.
    uint64_t addr;
    /// The buffer Length.
    uint32_t len;
    /// The buffer ID.
    uint16_t id;
    /// Flags.
    uint16_t flags;
} __packed;

struct virtq_event_suppress {
    uint16_t desc;
    uint16_t flags;
} __packed;

/// The maximum number of virtqueues.
#define NUM_VIRTQS_MAX 8

/// A virtqueue.
struct virtio_virtq {
    unsigned index;
    dma_t descs_dma;
    volatile struct virtq_desc *descs;
    size_t num_descs;
    dma_t buffers_dma;
    volatile void *buffers;
    offset_t queue_notify_off;
    int next_avail;
    int wrap_counter;
};

struct virtio_pci_common_cfg {
    uint32_t device_feature_select;
    uint32_t device_feature;
    uint32_t driver_feature_select;
    uint32_t driver_feature;
    uint16_t msix_config;
    uint16_t num_queues;
    uint8_t device_status;
    uint8_t config_generation;
    uint16_t queue_select;
    uint16_t queue_size;
    uint16_t queue_msix_vector;
    uint16_t queue_enable;
    uint16_t queue_notify_off;
    uint32_t queue_desc_lo;
    uint32_t queue_desc_hi;
    uint32_t queue_driver_lo;
    uint32_t queue_driver_hi;
    uint32_t queue_device_lo;
    uint32_t queue_device_hi;
} __packed;

#define VIRTIO_COMMON_CFG_READ(size, field) \
    io_read ## size(common_cfg_io, common_cfg_off + \
        offsetof(struct virtio_pci_common_cfg, field))
#define VIRTIO_COMMON_CFG_READ8(field) \
    VIRTIO_COMMON_CFG_READ(8, field)
#define VIRTIO_COMMON_CFG_READ16(field) \
    VIRTIO_COMMON_CFG_READ(16, field)
#define VIRTIO_COMMON_CFG_READ32(field) \
    VIRTIO_COMMON_CFG_READ(32, field)

#define VIRTIO_COMMON_CFG_WRITE(size, field, value) \
    io_write ## size(common_cfg_io, common_cfg_off + \
        offsetof(struct virtio_pci_common_cfg, field), value)
#define VIRTIO_COMMON_CFG_WRITE8(field, value) \
    VIRTIO_COMMON_CFG_WRITE(8, field, value)
#define VIRTIO_COMMON_CFG_WRITE16(field, value) \
    VIRTIO_COMMON_CFG_WRITE(16, field, value)
#define VIRTIO_COMMON_CFG_WRITE32(field, value) \
    VIRTIO_COMMON_CFG_WRITE(32, field, value)

#define VIRTIO_DEVICE_CFG_READ(size, struct_name, field) \
    io_read ## size(device_cfg_io, device_cfg_off + offsetof(struct_name, field))
#define VIRTIO_DEVICE_CFG_READ8(struct_name, field) \
    VIRTIO_DEVICE_CFG_READ(8, struct_name, field)
#define VIRTIO_DEVICE_CFG_READ16(struct_name, field) \
    VIRTIO_DEVICE_CFG_READ(16, struct_name, field)
#define VIRTIO_DEVICE_CFG_READ32(struct_name, field) \
    VIRTIO_DEVICE_CFG_READ(32, struct_name, field)

static uint8_t read_device_status(void) {
    return VIRTIO_COMMON_CFG_READ8(device_status);
}

static void write_device_status(uint8_t value) {
    VIRTIO_COMMON_CFG_WRITE8(device_status, value);
}

static void write_driver_feature(uint64_t value) {
    // Select and set feature bits 0 to 31.
    VIRTIO_COMMON_CFG_WRITE32(driver_feature_select, 0);
    VIRTIO_COMMON_CFG_WRITE32(driver_feature, value & 0xffffffff);

    // Select and set feature bits 32 to 63.
    VIRTIO_COMMON_CFG_WRITE32(driver_feature_select, 1);
    VIRTIO_COMMON_CFG_WRITE32(driver_feature, value >> 32);
}

static uint16_t read_num_virtq(void) {
    return VIRTIO_COMMON_CFG_READ16(num_queues);
}

static void virtq_select(unsigned index) {
    VIRTIO_COMMON_CFG_WRITE8(queue_select, index);
}

static uint16_t virtq_size(void) {
    return VIRTIO_COMMON_CFG_READ16(queue_size);
}

static void virtq_enable(void) {
    VIRTIO_COMMON_CFG_WRITE16(queue_enable, 1);
}

static void virtq_set_desc_paddr(uint64_t paddr) {
    VIRTIO_COMMON_CFG_WRITE32(queue_desc_lo, paddr & 0xffffffff);
    VIRTIO_COMMON_CFG_WRITE32(queue_desc_hi, paddr >> 32);
}

static void virtq_set_driver_paddr(uint64_t paddr) {
    VIRTIO_COMMON_CFG_WRITE32(queue_driver_lo, paddr & 0xffffffff);
    VIRTIO_COMMON_CFG_WRITE32(queue_driver_hi, paddr >> 32);
}

static void virtq_set_device_paddr(uint64_t paddr) {
    VIRTIO_COMMON_CFG_WRITE32(queue_device_lo, paddr & 0xffffffff);
    VIRTIO_COMMON_CFG_WRITE32(queue_device_hi, paddr >> 32);
}

static void virtq_notify(struct virtio_virtq *vq) {
    io_write16(notify_struct_io, vq->queue_notify_off, vq->index);
}

static int virtq_alloc(struct virtio_virtq *vq, size_t len) {
    int index = vq->next_avail;
    volatile struct virtq_desc *desc = &vq->descs[index];

    if (!!(desc->flags & VIRTQ_DESC_F_AVAIL) != !!(desc->flags & VIRTQ_DESC_F_USED)) {
        return -1;
    }

    desc->flags =
        (vq->wrap_counter << VIRTQ_DESC_F_AVAIL_SHIFT)
        | (!vq->wrap_counter << VIRTQ_DESC_F_USED_SHIFT);
    desc->len = len;

    if (vq->next_avail == vq->num_descs - 1) {
        vq->wrap_counter ^= 1;
        vq->next_avail = 0;
    } else {
        vq->next_avail++;
    }

    return index;
}

//
//  Driver
//
error_t driver_read_macaddr(uint8_t *mac) {
    mac[0] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac0);
    mac[1] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac1);
    mac[2] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac2);
    mac[3] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac3);
    mac[4] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac4);
    mac[5] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac5);
    return OK;
}

static struct virtio_virtq *tx_virtq = NULL;
static struct virtio_virtq *rx_virtq = NULL;

void driver_transmit(const uint8_t *payload, size_t len) {
    int index =
        virtq_alloc(tx_virtq, sizeof(struct virtio_net_header) + len);
    if (index < 0) {
        return;
    }

    struct virtio_net_buffer *buffers =
        (struct virtio_net_buffer *) tx_virtq->buffers;
    volatile struct virtio_net_buffer *buf = &buffers[index];
    if (!buf) {
        return;
    }

    ASSERT(len <= sizeof(buf->payload));
    buf->header.flags = 0;
    buf->header.gso_type = VIRTIO_NET_HDR_GSO_NONE;
    buf->header.gso_size = 0;
    buf->header.checksum_start = 0;
    buf->header.checksum_offset = 0;
    buf->header.num_buffers = 0;
    memcpy((uint8_t *) &buf->payload, payload, len);

    virtq_notify(tx_virtq);
}

void driver_handle_interrupt(void) {
    INFO("Handle IRQ");
}

error_t driver_init_for_pci(receive_callback_t receive) {
    return OK;
}

static struct net_driver_ops ops = {
    .init = driver_init_for_pci,
    .read_macaddr = driver_read_macaddr,
    .transmit = driver_transmit,
    .handle_interrupt = driver_handle_interrupt,
};

//
//  Driver Library
//
static task_t tcpip_tid;
static void receive(const void *payload, size_t len) {
    TRACE("received %d bytes", len);
    struct message m;
    m.type = NET_RX_MSG;
    m.net_rx.payload_len = len;
    m.net_rx.payload = (void *) payload;
    error_t err = ipc_send(tcpip_tid, &m);
    ASSERT_OK(err);
}

static void transmit(void) {
    struct message m;
    ASSERT_OK(async_recv(tcpip_tid, &m));
    ASSERT(m.type == NET_TX_MSG);
    INFO("transmitting...");
    driver_transmit(m.net_tx.payload, m.net_tx.payload_len);
    free((void *) m.net_tx.payload);
}

static task_t dm_server;

uint32_t pci_config_read(handle_t device, unsigned offset, unsigned size) {
    struct message m;
    m.type = DM_PCI_READ_CONFIG_MSG;
    m.dm_pci_read_config.handle = device;
    m.dm_pci_read_config.offset = offset;
    m.dm_pci_read_config.size = size;
    ASSERT_OK(ipc_call(dm_server, &m));
    return m.dm_pci_read_config_reply.value;
}

void main(void) {
    TRACE("starting...");

    // Look for the e1000...
    dm_server = ipc_lookup("dm");
    struct message m;
    m.type = DM_ATTACH_PCI_DEVICE_MSG;
    m.dm_attach_pci_device.vendor_id = 0x1af4;
    m.dm_attach_pci_device.device_id = 0x1000;
    ASSERT_OK(ipc_call(dm_server, &m));
    handle_t pci_device = m.dm_attach_pci_device_reply.handle;

    // Walk capabilities list. A capability consists of the following fields
    // (from "4.1.4 Virtio Structure PCI Capabilities"):
    //
    // struct virtio_pci_cap {
    //     u8 cap_vndr;    /* Generic PCI field: PCI_CAP_ID_VNDR */
    //     u8 cap_next;    /* Generic PCI field: next ptr. */
    //     u8 cap_len;     /* Generic PCI field: capability length */
    //     u8 cfg_type;    /* Identifies the structure. */
    //     u8 bar;         /* Where to find it. */
    //     u8 padding[3];  /* Pad to full dword. */
    //     le32 offset;    /* Offset within bar. */
    //     le32 length;    /* Length of the structure, in bytes. */
    // };
    uint8_t cap_off = pci_config_read(pci_device, 0x34, sizeof(uint8_t));
    while (cap_off != 0) {
        uint8_t cap_id = pci_config_read(pci_device, cap_off, sizeof(uint8_t));
        uint8_t cfg_type = pci_config_read(pci_device, cap_off + 3, sizeof(uint8_t));
        uint8_t bar_index = pci_config_read(pci_device, cap_off + 4, sizeof(uint8_t));

        TRACE("cap_id=%x, cfg_type=%x, bar_index=%d", cap_id, cfg_type, bar_index);

        if (cap_id == 9 && cfg_type == VIRTIO_PCI_CAP_COMMON_CFG) {
            uint32_t bar = pci_config_read(pci_device, 0x10 + 4 * bar_index, 4);
            ASSERT((bar & 1) == 0 && "only supports memory-mapped I/O access for now");
            uint32_t bar_base = bar & ~0xf;
            size_t size = pci_config_read(pci_device, cap_off + 12, 4);
            common_cfg_off = pci_config_read(pci_device, cap_off + 8, 4);
            common_cfg_io = io_alloc_memory_fixed(
                bar_base,
                ALIGN_UP(common_cfg_off + size, PAGE_SIZE),
                IO_ALLOC_CONTINUOUS
            );
        }

        if (cap_id == 9 && cfg_type == VIRTIO_PCI_CAP_DEVICE_CFG) {
            // Device-specific configuration space.
            uint32_t bar = pci_config_read(pci_device, 0x10 + 4 * bar_index, 4);
            ASSERT((bar & 1) == 0 && "only supports memory-mapped I/O access for now");
            uint32_t bar_base = bar & ~0xf;
            size_t size = pci_config_read(pci_device, cap_off + 12, 4);
            device_cfg_off = pci_config_read(pci_device, cap_off + 8, 4);
            device_cfg_io = io_alloc_memory_fixed(
                bar_base,
                ALIGN_UP(device_cfg_off + size, PAGE_SIZE),
                IO_ALLOC_CONTINUOUS
            );
        }

        if (cap_id == 9 && cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG) {
            // Notification structure:
            //
            // struct virtio_pci_notify_cap {
            //     struct virtio_pci_cap cap;
            //     le32 notify_off_multiplier; /* Multiplier for queue_notify_off. */
            // };
            notify_cap_off = pci_config_read(pci_device, cap_off + 8, 4);
            notify_off_multiplier =
                pci_config_read(pci_device, cap_off + 16, 4);
            uint32_t bar = pci_config_read(pci_device, 0x10 + 4 * bar_index, 4);
            ASSERT((bar & 1) == 0 && "only supports memory-mapped I/O access for now");
            uint32_t bar_base = bar & ~0xf;
            size_t size = pci_config_read(pci_device, cap_off + 12, 4);
            notify_struct_io = io_alloc_memory_fixed(
                bar_base,
                ALIGN_UP(notify_cap_off + size, PAGE_SIZE),
                IO_ALLOC_CONTINUOUS
            );
        }

        cap_off = pci_config_read(pci_device, cap_off + 1, sizeof(uint8_t));
    }

    ASSERT(common_cfg_io && device_cfg_io && notify_struct_io &&
        "failed to locate the BAR for the device access");

    // "3.1.1 Driver Requirements: Device Initialization"
    write_device_status(0); // Reset the device.
    write_device_status(read_device_status() | VIRTIO_STATUS_ACK);
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER);

    // Feature negotiation.
    uint32_t feats = VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS;
    write_driver_feature(feats | VIRTIO_F_VERSION_1 | VIRTIO_F_RING_PACKED);
    write_device_status(read_device_status() | VIRTIO_STATUS_FEAT_OK);
    ASSERT((read_device_status() & VIRTIO_STATUS_FEAT_OK) != 0);

    // Initialize virtqueues.
    unsigned num_virtq = read_num_virtq();
    ASSERT(num_virtq < NUM_VIRTQS_MAX);
    struct virtio_virtq virtqs[NUM_VIRTQS_MAX];
    for (unsigned i = 0; i < num_virtq; i++) {
        virtq_select(i);
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
        virtq_set_desc_paddr(dma_daddr(descs_dma));
        virtq_set_driver_paddr(dma_daddr(driver_dma));
        virtq_set_device_paddr(dma_daddr(device_dma));
        virtq_enable();

        virtqs[i].index = i;
        virtqs[i].descs_dma = descs_dma;
        virtqs[i].descs = (struct virtq_desc *) dma_buf(descs_dma);
        virtqs[i].num_descs = num_descs;
        virtqs[i].queue_notify_off = queue_notify_off;
        virtqs[i].next_avail = 0;
        virtqs[i].wrap_counter = 1;
    }

    // Allocate RX buffers.
    size_t buffer_size = sizeof(struct virtio_net_buffer);
    rx_virtq = &virtqs[VIRTIO_NET_QUEUE_RX];
    dma_t rx_dma = dma_alloc(
        buffer_size * rx_virtq->num_descs,
        DMA_ALLOC_FROM_DEVICE
    );
    rx_virtq->buffers_dma = rx_dma;
    rx_virtq->buffers = dma_buf(rx_dma);
    for (size_t i = 0; i < rx_virtq->num_descs; i++) {
        rx_virtq->descs[i].addr = dma_daddr(rx_dma) + (buffer_size * i);
    }

    // Allocate TX buffers.
    tx_virtq = &virtqs[VIRTIO_NET_QUEUE_TX];
    dma_t tx_dma = dma_alloc(
        buffer_size * tx_virtq->num_descs,
        DMA_ALLOC_FROM_DEVICE
    );
    tx_virtq->buffers_dma = tx_dma;
    tx_virtq->buffers = dma_buf(tx_dma);
    for (size_t i = 0; i < tx_virtq->num_descs; i++) {
        tx_virtq->descs[i].addr = dma_daddr(tx_dma) + (buffer_size * i);
    }

    // Make the device active.
    DBG("device active");
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER_OK);
    DBG("device actived");

    uint8_t mac[6];
    driver_read_macaddr((uint8_t *) &mac);
    INFO("initialized the device");
    INFO("MAC address = %02x:%02x:%02x:%02x:%02x:%02x",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ASSERT_OK(ipc_serve("net"));

    // Register this driver.
    tcpip_tid = ipc_lookup("tcpip");
    ASSERT_OK(tcpip_tid);
    m.type = TCPIP_REGISTER_DEVICE_MSG;
    memcpy(m.tcpip_register_device.macaddr, mac, 6);
    ASSERT_OK(ipc_call(tcpip_tid, &m));

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_IRQ) {
                    driver_handle_interrupt();
                }

                if (m.notifications.data & NOTIFY_ASYNC) {
                    transmit();
                }
                break;
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
