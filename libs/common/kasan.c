#include <kasan.h>
#include <print_macros.h>

void check_address(vaddr_t addr, size_t size) {
    if(size < 8) {
        if(shadow[addr>>3] && shadow[addr>>3] <= (addr & 7) + size - 1) {
            PANIC("ASan: detected an invalid access to %p", addr);
        }
    }
    else {
        if(size%8) {
            PANIC("ASan: size is not a multiple of 8");
        }
        for(size_t i = 0; i < addr/8; i++) {
            if(shadow[(addr>>3)+i]) {
                PANIC("ASan: detected an invalid access to %p", addr);
            }
        }
    }
}

void __asan_handle_no_return(void) {
}

#ifdef KERNEL
// We don't support KASan in kernel space for now.
void __asan_load8_noabort(vaddr_t addr) {
}

void __asan_store8_noabort(vaddr_t addr) {
}

void __asan_load4_noabort(vaddr_t addr) {
}

void __asan_store4_noabort(vaddr_t addr) {
}

void __asan_load2_noabort(vaddr_t addr) {
}

void __asan_store2_noabort(vaddr_t addr) {
}

void __asan_load1_noabort(vaddr_t addr) {
}

void __asan_store1_noabort(vaddr_t addr) {
}

void __asan_loadN_noabort(vaddr_t addr, size_t size) {
}

void __asan_storeN_noabort(vaddr_t addr, size_t size) {
}

#else
void __asan_load8_noabort(vaddr_t addr) {
    check_address(addr, 8);
    // if (shadow[addr>>3] <= 0) {
    //     PANIC("ASan: detected an invalid access to %p", addr);
    // }
}

void __asan_store8_noabort(vaddr_t addr) {
    check_address(addr, 8);
}

void __asan_load4_noabort(vaddr_t addr) {
    check_address(addr, 4);
    // if(addr%8 < shadow[addr>>3]) {
    //     PANIC("ASan: detected an invalid access to %p", addr);
    // }
}

void __asan_store4_noabort(vaddr_t addr) {
    check_address(addr, 4);
}

void __asan_load2_noabort(vaddr_t addr) {
    check_address(addr, 2);
}

void __asan_store2_noabort(vaddr_t addr) {
    check_address(addr, 2);
}

void __asan_load1_noabort(vaddr_t addr) {
    check_address(addr, 1);
}

void __asan_store1_noabort(vaddr_t addr) {
    check_address(addr, 1);
}

void __asan_loadN_noabort(vaddr_t addr, size_t size) {
    check_address(addr, size);
}

void __asan_storeN_noabort(vaddr_t addr, size_t size) {
    check_address(addr, size);
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
