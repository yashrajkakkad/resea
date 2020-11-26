#include<kasan.h>

// Stores the current state of the each memory bytes.
#define NUM_BYTES 512*1024
uint8_t shadow[NUM_BYTES];  /* .bss size + .data size + heap size */

#ifdef KERNEL
// We don't support KASan in kernel space for now.
void __asan_load8_noabort(vaddr_t addr) {
}
#else
void __asan_load8_noabort(vaddr_t addr) {
    if (!shadow[addr]) {
        PANIC("ASan: detected an invalid access to %p", addr);
    }
}
#endif
