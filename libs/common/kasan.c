#include<kasan.h>


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

// void encode_shadow(void *addr, size_t len)
// {
//     if(len % 8)
//     {
//         PANIC("ASan: Length specified for shadow is not a multiple of 8");
//     }
//     shadow[(paddr_t)addr>>3] = 0;
// }

// void shadow_chunk()
