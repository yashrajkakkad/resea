#include <driver/io.h>

void io_out8(io_t io, offset_t offset, uint8_t value) {
    switch (io->space) {
        case IO_SPACE_IO:
#ifdef ARCH_X64
            __asm__ __volatile__("outb %0, %1" :: "a"(value), "Nd"(port));
            break;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        case IO_SPACE_MEMORY:
            *((volatile uint8_t *) (io->memory.base + offset)) = value;
            break;
    }
}

void io_out16(io_t io, offset_t offset, uint16_t value) {
    switch (io->space) {
        case IO_SPACE_IO:
#ifdef ARCH_X64
            __asm__ __volatile__("outw %0, %1" :: "a"(value), "Nd"(port));
            break;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        case IO_SPACE_MEMORY:
            *((volatile uint16_t *) (io->memory.base + offset)) = value;
            break;
    }
}

void io_out32(io_t io, offset_t offset, uint32_t value) {
    switch (io->space) {
        case IO_SPACE_IO:
#ifdef ARCH_X64
            __asm__ __volatile__("outl %0, %1" :: "a"(value), "Nd"(port));
            break;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        case IO_SPACE_MEMORY:
            *((volatile uint32_t *) (io->memory.base + offset)) = value;
            break;
    }
}

uint8_t io_in8(io_t io, offset_t offset) {
    switch (io->space) {
        case IO_SPACE_IO: {
#ifdef ARCH_X64
            uint8_t value;
            __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
            return value;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        }
        case IO_SPACE_MEMORY:
            return *((volatile uint8_t *) (io->memory.base + offset));
    }
}

uint16_t io_in16(io_t io, offset_t offset) {
    switch (io->space) {
        case IO_SPACE_IO: {
#ifdef ARCH_X64
            uint16_t value;
            __asm__ __volatile__("inw %1, %0" : "=a"(value) : "Nd"(port));
            return value;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        }
        case IO_SPACE_MEMORY:
            return *((volatile uint16_t *) (io->memory.base + offset));
    }
}

uint32_t io_in32(io_t io, offset_t offset) {
    switch (io->space) {
        case IO_SPACE_IO: {
#ifdef ARCH_X64
            uint32_t value;
            __asm__ __volatile__("inl %1, %0" : "=a"(value) : "Nd"(port));
            return value;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        }
        case IO_SPACE_MEMORY:
            return *((volatile uint32_t *) (io->memory.base + offset));
    }
}
