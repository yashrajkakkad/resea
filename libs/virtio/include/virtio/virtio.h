#ifndef __VIRTIO_VIRTIO_H__
#define __VIRTIO_VIRTIO_H__

#include <types.h>
#include <driver/dma.h>


//
//  "5 Device Types"
//
#define VIRTIO_DEVICE_NET      1

//
//  "2.1 Device Status Field"
//
#define VIRTIO_STATUS_ACK        1
#define VIRTIO_STATUS_DRIVER     2
#define VIRTIO_STATUS_DRIVER_OK  4
#define VIRTIO_STATUS_FEAT_OK    82

#define VIRTIO_F_VERSION_1       (1ull << 32)
#define VIRTIO_F_RING_PACKED     (1ull << 34)

/// A virtqueue.
struct virtio_virtq {
    /// The virtqueue index.
    unsigned index;
    /// Descriptors.
    dma_t descs_dma;
    struct virtq_desc *descs;
    /// The number of descriptors.
    int num_descs;
    /// Static buffers referenced from descriptors.
    dma_t buffers_dma;
    void *buffers;
    /// The size of a buffer.
    size_t buffer_size;
    /// The queue notify offset for the queue.
    offset_t queue_notify_off;
    /// The next descriptor index to be allocated.
    int next_avail;
    /// The next descriptor index to be used by the device.
    int next_used;
    /// Driver-side wrapping counter.
    int avail_wrap_counter;
    /// Device-side wrapping counter.
    int used_wrap_counter;
};

#define VIRTQ_DESC_F_AVAIL_SHIFT  7
#define VIRTQ_DESC_F_USED_SHIFT   15
#define VIRTQ_DESC_F_AVAIL        (1 << VIRTQ_DESC_F_AVAIL_SHIFT)
#define VIRTQ_DESC_F_USED         (1 << VIRTQ_DESC_F_USED_SHIFT)
#define VIRTQ_DESC_F_WRITE        2

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

struct virtio_ops {
    uint64_t (*read_device_features)(void);
    void (*negotiate_feature)(uint64_t features);
    void (*init_virtqueues)(void);
    uint64_t (*read_device_config)(offset_t offset, size_t size);
    void (*activate)(void);
    uint8_t (*read_isr_status)(void);
    struct virtio_virtq *(*virtq_get)(unsigned index);
    uint16_t (*virtq_size)(void);
    void (*virtq_allocate_buffers)(struct virtio_virtq *vq, size_t buffer_size, bool writable);
    int (*virtq_alloc)(struct virtio_virtq *vq, size_t len);
    struct virtq_desc *(*virtq_pop_desc)(struct virtio_virtq *vq);
    void (*virtq_push_desc)(struct virtio_virtq *vq, struct virtq_desc *desc);
    void (*virtq_notify)(struct virtio_virtq *vq);
};

error_t virtio_find_device(int device_type, struct virtio_ops **ops, uint8_t *irq);

#endif
