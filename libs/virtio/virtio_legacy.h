#ifndef __VIRTIO_VIRTIO_LEGACY_H__
#define __VIRTIO_VIRTIO_LEGACY_H__

#include <types.h>

struct virtio_virtq_legacy {
};

struct virtio_ops;
error_t virtio_legacy_find_device(int device_type, struct virtio_ops **ops, uint8_t *irq);

#endif
