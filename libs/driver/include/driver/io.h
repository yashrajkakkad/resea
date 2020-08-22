#ifndef __DRIVER_IO_H__
#define __DRIVER_IO_H__

#include <types.h>

enum io_space {
    IO_SPACE_IO,
    IO_SPACE_MEMORY,
};

struct io {
    enum io_space space;
    union {
        struct {
            unsigned long base;
        } port;
        struct {
            vaddr_t vaddr;
            paddr_t paddr;
        } memory;
    };
};

typedef struct io *io_t;

#define IO_ALLOC_NORMAL     0
#define IO_ALLOC_CONTINUOUS 1

io_t io_alloc_port(unsigned long base, size_t len, unsigned flags);
io_t io_alloc_memory(size_t len, unsigned flags);
io_t io_alloc_memory_fixed(paddr_t paddr, size_t len, unsigned flags);
paddr_t io_paddr(io_t io);
vaddr_t io_vaddr(io_t io);
void io_write8(io_t io, offset_t offset, uint8_t value);
void io_write16(io_t io, offset_t offset, uint16_t value);
void io_write32(io_t io, offset_t offset, uint32_t value);
void io_write64(io_t io, offset_t offset, uint64_t value);
uint8_t io_read8(io_t io, offset_t offset);
uint16_t io_read16(io_t io, offset_t offset);
uint32_t io_read32(io_t io, offset_t offset);
uint64_t io_read64(io_t io, offset_t offset);
void io_read_bytes(io_t io, offset_t offset, const uint8_t *buf, size_t len);
void io_write_bytes(io_t io, offset_t offset, const uint8_t *buf, size_t len);
void io_free(io_t io, offset_t offset);

#endif
