#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <driver/io.h>
#include <driver/dma.h>
#include <string.h>
#include "e1000.h"

io_t regs_io;
static io_t rx_descs_io;
static io_t tx_descs_io;
static dma_t rx_buffers_dma;
static dma_t tx_buffers_dma;
static struct rx_desc *rx_descs;
static struct tx_desc *tx_descs;
static struct buffer *rx_buffers;
static struct buffer *tx_buffers;
static uint32_t tx_current;
static uint32_t rx_current;

static uint8_t read_reg8(uint32_t offset) {
    uint32_t aligned_offset = offset & 0xfffffffc;
    uint32_t value = io_read32(regs_io, aligned_offset);
    return (value >> ((offset & 0x03) * 8)) & 0xff;
}

void e1000_transmit(const void *pkt, size_t len) {
    ASSERT(len <= BUFFER_SIZE);

    // Copy the packet into the TX buffer.
    memcpy(tx_buffers[tx_current].data, pkt, len);
    dma_flush_write(tx_buffers_dma);

    // Set descriptor fields. The buffer address field is already filled
    // during initialization.
    struct tx_desc *desc = &tx_descs[tx_current];
    desc->cmd = TX_DESC_IFCS | TX_DESC_EOP;
    desc->len = len;
    desc->cso = 0;
    desc->status = 0;
    desc->css = 0;
    desc->special = 0;

    // Notify the device.
    tx_current = (tx_current + 1) % NUM_TX_DESCS;
    io_write32(regs_io, REG_TDT, tx_current);
    io_flush_write(regs_io);

    // TODO: dma_read(tx_buffers_dma);

    TRACE("sent %d bytes", len);
}

void e1000_handle_interrupt(void (*receive)(const void *payload, size_t len)) {
    io_flush_read(regs_io);
    uint32_t cause = io_read32(regs_io, REG_ICR);
    if ((cause & ICR_RXT0) != 0) {
        while (true) {
            struct rx_desc *desc = &rx_descs[rx_current];

            // We don't support a large packet which spans multiple descriptors.
            uint8_t bits = RX_DESC_DD | RX_DESC_EOP;
            if ((desc->status & bits) != bits) {
                break;
            }

            dma_flush_read(rx_buffers_dma);
            receive(rx_buffers[rx_current].data, desc->len);

            // Tell the device that we've tasked a received packet.
            rx_descs[rx_current].status = 0;
            io_write32(regs_io, REG_RDT, rx_current);
            io_flush_write(regs_io);
            rx_current = (rx_current + 1) % (NUM_RX_DESCS);
        }
    }
}

void e1000_read_macaddr(uint8_t *macaddr) {
    io_flush_read(regs_io);

    for (int i = 0; i < 4; i++) {
        macaddr[i] = read_reg8(REG_RECEIVE_ADDR_LOW + i);
    }

    for (int i = 0; i < 2; i++) {
        macaddr[4 + i] = read_reg8(REG_RECEIVE_ADDR_HIGH + i);
    }
}

static void e1000_init(void) {
    rx_descs = (struct rx_desc *) io_vaddr(rx_descs_io);
    tx_descs = (struct tx_desc *) io_vaddr(tx_descs_io);
    rx_buffers = (struct buffer *) dma_vaddr(rx_buffers_dma);
    tx_buffers = (struct buffer *) dma_vaddr(tx_buffers_dma);
    paddr_t rx_descs_paddr = io_paddr(rx_descs_io);
    paddr_t tx_descs_paddr = io_paddr(tx_descs_io);
    paddr_t rx_buffers_paddr = dma_daddr(rx_buffers_dma);
    paddr_t tx_buffers_paddr = dma_daddr(tx_buffers_dma);

    // Reset the device.
    io_write32(regs_io, REG_CTRL, io_read32(regs_io, REG_CTRL) | CTRL_RST);

    // Wait until the device gets reset.
    while ((io_read32(regs_io, REG_CTRL) & CTRL_RST) != 0) {}

    // Link up!
    io_write32(regs_io, REG_CTRL, io_read32(regs_io, REG_CTRL) | CTRL_SLU | CTRL_ASDE);

    // Fill Multicast Table Array with zeros.
    for (int i = 0; i < 0x80; i++) {
        io_write32(regs_io, REG_MTA_BASE + i * 4, 0);
    }

    // Initialize RX queue.
    for (int i = 0; i < NUM_RX_DESCS; i++) {
        rx_descs[i].paddr = rx_buffers_paddr + i * BUFFER_SIZE;
        rx_descs[i].status = 0;
    }

    io_write32(regs_io, REG_RDBAL, rx_descs_paddr & 0xffffffff);
    io_write32(regs_io, REG_RDBAH, rx_descs_paddr >> 32);
    io_write32(regs_io, REG_RDLEN, NUM_RX_DESCS * sizeof(struct rx_desc));
    io_write32(regs_io, REG_RDH, 0);
    io_write32(regs_io, REG_RDT, NUM_RX_DESCS);
    io_write32(regs_io, REG_RCTL, RCTL_EN | RCTL_SECRC | RCTL_BSIZE | RCTL_BAM);

    // Initialize TX queue.
    for (int i = 0; i < NUM_TX_DESCS; i++) {
        tx_descs[i].paddr = tx_buffers_paddr + i * BUFFER_SIZE;
    }

    io_write32(regs_io, REG_TDBAL, tx_descs_paddr & 0xffffffff);
    io_write32(regs_io, REG_TDBAH, tx_descs_paddr >> 32);
    io_write32(regs_io, REG_TDLEN, NUM_TX_DESCS * sizeof(struct tx_desc));
    io_write32(regs_io, REG_TDH, 0);
    io_write32(regs_io, REG_TDT, 0);
    io_write32(regs_io, REG_TCTL, TCTL_EN | TCTL_PSP);

    // Enable interrupts.
    io_write32(regs_io, REG_IMS, 0xff);
    io_write32(regs_io, REG_IMC, 0xff);
    io_write32(regs_io, REG_IMS, IMS_RXT0);
    io_flush_write(regs_io);
}


void e1000_init_for_pci(uint32_t bar0_addr, uint32_t bar0_len) {
    regs_io = io_alloc_memory_fixed(bar0_addr, bar0_len, IO_ALLOC_CONTINUOUS);
    rx_descs_io = io_alloc_memory(sizeof(struct rx_desc) * NUM_RX_DESCS,
                                  IO_ALLOC_CONTINUOUS);
    tx_descs_io = io_alloc_memory(sizeof(struct tx_desc) * NUM_TX_DESCS,
                                  IO_ALLOC_CONTINUOUS);
    rx_buffers_dma = dma_alloc(BUFFER_SIZE * NUM_RX_DESCS,
                               DMA_ALLOC_FROM_DEVICE);
    tx_buffers_dma = dma_alloc(BUFFER_SIZE * NUM_TX_DESCS,
                               DMA_ALLOC_TO_DEVICE);

    e1000_init();
}
