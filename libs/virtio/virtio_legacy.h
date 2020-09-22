#ifndef __VIRTIO_VIRTIO_LEGACY_H__
#define __VIRTIO_VIRTIO_LEGACY_H__

#include <types.h>

#define REG_DEVICE_FEATS   0x00
#define REG_DRIVER_FEATS   0x04
#define REG_QUEUE_ADDR     0x08
#define REG_QUEUE_SIZE     0x0c
#define REG_QUEUE_SELECT   0x0e
#define REG_QUEUE_NOTIFY   0x10
#define REG_DEVICE_STATUS  0x12
#define REG_ISR_STATUS     0x13

struct virtio_virtq_legacy {
};

struct virtio_ops;
error_t virtio_legacy_find_device(int device_type, struct virtio_ops **ops, uint8_t *irq);

#endif
