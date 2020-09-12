#include <resea/ipc.h>
#include <resea/async.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <driver/irq.h>
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

void main(void) {
    TRACE("starting...");

    // Look for the e1000...
    task_t dm_server = ipc_lookup("dm");
    struct message m;
    m.type = DM_ATTACH_PCI_DEVICE_MSG;
    m.dm_attach_pci_device.vendor_id = 0x1af4;
    m.dm_attach_pci_device.device_id = 0x1000;
    ASSERT_OK(ipc_call(dm_server, &m));
    handle_t device = m.dm_attach_pci_device_reply.handle;

    m.type = DM_PCI_GET_CONFIG_MSG;
    m.dm_pci_get_config.handle = device;
    ASSERT_OK(ipc_call(dm_server, &m));
    uint32_t bar0_addr = m.dm_pci_get_config_reply.bar0_addr;
    uint32_t bar0_len = m.dm_pci_get_config_reply.bar0_len;
    uint8_t irq = m.dm_pci_get_config_reply.irq;

    m.type = DM_PCI_ENABLE_BUS_MASTER_MSG;
    m.dm_pci_enable_bus_master.handle = device;
    ASSERT_OK(ipc_call(dm_server, &m));

    ASSERT_OK(irq_acquire(irq));
//    driver_init_for_pci(bar0_addr, bar0_len);

    uint8_t mac[6];
    driver_read_macaddr((uint8_t *) &mac);
    INFO("initialized the device");
    INFO("MAC address = %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
         mac[3], mac[4], mac[5]);

    // Wait for the tcpip server.
    tcpip_tid = ipc_lookup("tcpip");
    ASSERT_OK(tcpip_tid);

    ASSERT_OK(ipc_serve("net"));

    // Register this driver.
    /*
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
