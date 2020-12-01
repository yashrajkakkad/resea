#include<types.h>

void __asan_load8_noabort(vaddr_t addr);
// Stores the current state of the each memory bytes.
#define NUM_BYTES 512*1024
uint8_t shadow[NUM_BYTES];  /* .bss size + .data size + heap size */

#define SHADOW_UNADDRESSABLE -1
// #define SHADOW_NEXT_PTR -1
// #define SHADOW_CAPACITY -2
// #define SHADOW_SIZE -3
// #define SHADOW_MAGIC -4
// #define SHADOW_UNDERFLOW_REDZONE -5
// #define SHADOW_DATA -6
