#ifndef __RESEA_KASAN_H__
#define __RESEA_KASAN_H__

#include<types.h>
extern char __heap[];
extern char __heap_end[];
extern char __stack[];
extern char __stack_end[];

void __asan_load8_noabort(vaddr_t addr);
void __asan_store8_noabort(vaddr_t addr);
void __asan_load4_noabort(vaddr_t addr);
void __asan_store4_noabort(vaddr_t addr);
void __asan_load2_noabort(vaddr_t addr);
void __asan_store2_noabort(vaddr_t addr);
void __asan_load1_noabort(vaddr_t addr);
void __asan_store1_noabort(vaddr_t addr);
void __asan_loadN_noabort(vaddr_t addr, size_t size);
void __asan_storeN_noabort(vaddr_t addr, size_t size);
void __asan_handle_no_return(void);
void check_address(vaddr_t addr, size_t size);
// Stores the current state of the each memory bytes.
// #define NUM_BYTES (__heap_end - __heakp) + (__stack_end - __stack);
#define NUM_BYTES 0x24000
extern char __shadow[];  /* .bss size + .data size + heap size */


#define SHADOW_UNADDRESSABLE -1
#define SHADOW_NEXT_PTR -2
#define SHADOW_CAPACITY -3
#define SHADOW_SIZE -4
#define SHADOW_MAGIC -5
#define SHADOW_UNDERFLOW_REDZONE -6
#define SHADOW_DATA -7
#define SHADOW_FREED -8

#endif
