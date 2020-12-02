#include<types.h>
extern char __heap[];
extern char __heap_end[];
extern char __stack[];
extern char __stack_end[];

void __asan_load8_noabort(vaddr_t addr);
// Stores the current state of the each memory bytes.
// #define NUM_BYTES (__heap_end - __heap) + (__stack_end - __stack);
#define NUM_BYTES 0x105000
uint8_t shadow[NUM_BYTES];  /* .bss size + .data size + heap size */
#define SHADOW_UNADDRESSABLE -1
#define SHADOW_NEXT_PTR -2
#define SHADOW_CAPACITY -3
#define SHADOW_SIZE -4
#define SHADOW_MAGIC -5
#define SHADOW_UNDERFLOW_REDZONE -6
#define SHADOW_DATA -7
