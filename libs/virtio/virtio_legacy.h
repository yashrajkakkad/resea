#ifndef __VIRTIO_VIRTIO_LEGACY_H__
#define __VIRTIO_VIRTIO_LEGACY_H__

#include <types.h>

#define REG_DEVICE_FEATS   0x00
#define REG_DRIVER_FEATS   0x04
#define REG_QUEUE_ADDR     0x08
#define REG_NUM_DESCS     0x0c
#define REG_QUEUE_SELECT   0x0e
#define REG_QUEUE_NOTIFY   0x10
#define REG_DEVICE_STATUS  0x12
#define REG_ISR_STATUS     0x13

struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
} __packed;

/*uint32_t is used here for ids for padding reasons. */
struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __packed;

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[];
} __packed;


struct virtio_virtq_legacy {
    dma_t virtq_dma;
    int num_descs;
    struct virtq_desc *descs;
    struct virtq_avail *avail;
    struct virtq_used *used;
};

struct virtio_ops;
error_t virtio_legacy_find_device(int device_type, struct virtio_ops **ops, uint8_t *irq);

#endif
