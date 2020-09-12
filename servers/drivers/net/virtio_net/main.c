#include <resea/ipc.h>
#include <resea/async.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <driver/irq.h>
#include <driver/io.h>
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
static offset_t common_cfg_base;

// "2.1 Device Status Field"
#define VIRTIO_STATUS_ACK        1
#define VIRTIO_STATUS_DRIVER     2
#define VIRTIO_STATUS_DRIVER_OK  4
#define VIRTIO_STATUS_FEAT_OK    8

#define VIRTIO_F_VERSION_1       (1ull << 32)

#define VIRTIO_NET_F_MAC         (1 << 5)
#define VIRTIO_NET_F_STATUS      (1 << 16)

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
    uint64_t queue_desc;
    uint64_t queue_driver;
    uint64_t queue_device;
} __packed;

static uint8_t read_device_status(void) {
    offset_t off = offsetof(struct virtio_pci_common_cfg, device_status);
    return io_read8(common_cfg_io, common_cfg_base + off);
}

static void write_device_status(uint8_t value) {
    offset_t off = offsetof(struct virtio_pci_common_cfg, device_status);
    return io_write8(common_cfg_io, common_cfg_base + off, value);
}

static void write_driver_feature(uint64_t value) {
    offset_t select_off = offsetof(struct virtio_pci_common_cfg, driver_feature_select);
    offset_t off = offsetof(struct virtio_pci_common_cfg, driver_feature);

    // Select and set feature bits 32 to 63.
    io_write32(common_cfg_io, common_cfg_base + select_off, 1);
    io_write32(common_cfg_io, common_cfg_base + off, value >> 32);

    // Select and set feature bits 0 to 31.
    io_write32(common_cfg_io, common_cfg_base + select_off, 0);
    io_write32(common_cfg_io, common_cfg_base + off, value & 0xffffffff);
}

//
//  Driver
//
error_t driver_read_macaddr(uint8_t *mac) {
    return OK;
}

void driver_transmit(const uint8_t *payload, size_t len) {
}

void driver_handle_interrupt() {
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
    INFO("cap_off = %d", cap_off);
    uint32_t bar_base = 0, bar_offset, bar_len;
    while (cap_off != 0) {
        uint8_t cap_id = pci_config_read(pci_device, cap_off, sizeof(uint8_t));
        uint8_t cfg_type = pci_config_read(pci_device, cap_off + 3, sizeof(uint8_t));
        uint8_t bar_index = pci_config_read(pci_device, cap_off + 4, sizeof(uint8_t));
        TRACE("cap_id=%x, cfg_type=%x, bar_index=%d", cap_id, cfg_type, bar_index);
        if (cap_id == 9 && cfg_type == 1) {
            // From "4.1.4.3 Common configuration structure layout":
            //
            // struct virtio_pci_common_cfg {
            //     /* About the whole device. */
            //     le32 device_feature_select;     /* read-write */
            //     le32 device_feature;            /* read-only for driver */
            //     le32 driver_feature_select;     /* read-write */
            //     le32 driver_feature;            /* read-write */
            //     le16 msix_config;               /* read-write */
            //     le16 num_queues;                /* read-only for driver */
            //     u8 device_status;               /* read-write */
            //     u8 config_generation;           /* read-only for driver */
            //
            //     /* About a specific virtqueue. */
            //     le16 queue_select;              /* read-write */
            //     le16 queue_size;                /* read-write */
            //     le16 queue_msix_vector;         /* read-write */
            //     le16 queue_enable;              /* read-write */
            //     le16 queue_notify_off;          /* read-only for driver */
            //     le64 queue_desc;                /* read-write */
            //     le64 queue_driver;              /* read-write */
            //     le64 queue_device;              /* read-write */
            // };

            uint32_t bar = pci_config_read(pci_device, 0x10 + 4 * bar_index, 4);
            ASSERT((bar & 1) == 0 && "only supports memory-mapped I/O access for now");
            bar_base = bar & ~0xf;
            bar_offset = pci_config_read(pci_device, cap_off + 8, 4);
            bar_len = pci_config_read(pci_device, cap_off + 12, 4);
        }

        cap_off = pci_config_read(pci_device, cap_off + 1, sizeof(uint8_t));
    }

    if (!bar_base) {
        PANIC("failed to locate the BAR for the device access");
    }

    INFO("bar: base=%p, offset=%x, len=%d", bar_base, bar_offset, bar_len);
    common_cfg_base = bar_offset;
    common_cfg_io = io_alloc_memory_fixed(bar_base,
                                          bar_len + bar_offset, IO_ALLOC_CONTINUOUS);

    uint16_t num_queues = io_read16(common_cfg_io, common_cfg_base + offsetof(struct virtio_pci_common_cfg, num_queues));
    DBG("nq = %d", num_queues);

    // "3.1.1 Driver Requirements: Device Initialization"
    write_device_status(0); // Reset the device.
    write_device_status(read_device_status() | VIRTIO_STATUS_ACK);
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER);

    // Feature negotiation.
    uint32_t feats = VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS;
    write_driver_feature(VIRTIO_F_VERSION_1 | feats);

    write_device_status(read_device_status() | VIRTIO_STATUS_FEAT_OK);
    ASSERT((read_device_status() & VIRTIO_STATUS_FEAT_OK) != 0);


    // Make the device active.
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER_OK);


//    driver_init_for_pci(bar0_addr, bar0_len);

    uint8_t mac[6];
    driver_read_macaddr((uint8_t *) &mac);
    INFO("initialized the device");
    INFO("MAC address = %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
         mac[3], mac[4], mac[5]);


    ASSERT_OK(ipc_serve("net"));

    // Register this driver.
    /*
    // Wait for the tcpip server.
    tcpip_tid = ipc_lookup("tcpip");
    ASSERT_OK(tcpip_tid);
    m.type = TCPIP_REGISTER_DEVICE_MSG;
    memcpy(m.tcpip_register_device.macaddr, mac, 6);
    err = ipc_call(tcpip_tid, &m);
    ASSERT_OK(err);
    */

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
