#include <list.h>
#include <resea/malloc.h>
#include <kasan.h>
#include <resea/printf.h>
#include <string.h>


#define NUM_BINS 16
extern char __heap[];
extern char __heap_end[];
extern char __stack[];
extern char __stack_end[];
extern char __shadow[];  /* .bss size + .data size + heap size */
extern char __shadow_end[];
// extern char __shadow[];

static struct malloc_chunk *bins[NUM_BINS];

static void check_buffer_overflow(struct malloc_chunk *chunk) {
    if (chunk->magic == MALLOC_FREE) {
        return;
    }

    for (size_t i = 0; i < MALLOC_REDZONE_LEN; i++) {
        if (chunk->underflow_redzone[i] != MALLOC_REDZONE_UNDFLOW_MARKER) {
            PANIC("detected a malloc buffer underflow: ptr=%p", chunk->data);
        }
    }

    for (size_t i = 0; i < MALLOC_REDZONE_LEN; i++) {
        if (chunk->data[chunk->capacity + i] != MALLOC_REDZONE_OVRFLOW_MARKER) {
            PANIC("detected a malloc buffer overflow: ptr=%p", chunk->data);
        }
    }
}

static struct malloc_chunk *insert(void *ptr, size_t len) {
    ASSERT(len > MALLOC_FRAME_LEN);
    struct malloc_chunk *new_chunk = ptr;
    new_chunk->magic = MALLOC_FREE;
    new_chunk->capacity = len - MALLOC_FRAME_LEN;
    new_chunk->size = 0;
    new_chunk->next = NULL;

    // Append the new chunk into the linked list.
    struct malloc_chunk **chunk = &bins[NUM_BINS - 1];
    while (*chunk != NULL) {
        check_buffer_overflow(*chunk);
        chunk = &(*chunk)->next;
    }
    *chunk = new_chunk;

    return new_chunk;
}

static struct malloc_chunk *split(struct malloc_chunk *chunk, size_t len) {
    size_t new_chunk_len = MALLOC_FRAME_LEN + len;
    ASSERT(chunk->capacity >= new_chunk_len);

    void *new_chunk_ptr =
        &chunk->data[chunk->capacity + MALLOC_REDZONE_LEN - new_chunk_len];
    chunk->capacity -= new_chunk_len;

    ASSERT(new_chunk_len > MALLOC_FRAME_LEN);

    struct malloc_chunk *new_chunk = new_chunk_ptr;
    new_chunk->magic = MALLOC_FREE;
    new_chunk->capacity = len;
    new_chunk->size = 0;
    new_chunk->next = NULL;

    return new_chunk;
}

static int get_bin_idx_from_size(size_t size) {
    // If requested size is less or equal to the size of second largest chunk
    // (the last fixed chunk).
    for (size_t i = 0; i < NUM_BINS - 1; i++) {
        if (size <= 1 << i) {
            return i;
        }
    }

    // Return -1 indicating the last, dynamic-sized chunk
    return -1;
}

void *malloc(size_t size) {
    if (!size) {
        size = 1;
    }

    // Align up to 16-bytes boundary. If the size is less than 16 (including
    // size == 0), allocate 16 bytes.
    size = ALIGN_UP(size, 16);

    int bin_idx = get_bin_idx_from_size(size);

    if (bin_idx != -1 && bins[bin_idx] != NULL) {
        // Check the list corresponding to that size for a free chunk.
        struct malloc_chunk *allocated = bins[bin_idx];
        ASSERT(allocated->magic == MALLOC_FREE);

        allocated->magic = MALLOC_IN_USE;
        allocated->size = size;
        memset(allocated->underflow_redzone, MALLOC_REDZONE_UNDFLOW_MARKER,
                MALLOC_REDZONE_LEN);
        memset(&allocated->data[allocated->capacity],
                MALLOC_REDZONE_OVRFLOW_MARKER, MALLOC_REDZONE_LEN);

        bins[bin_idx] = allocated->next;
        allocated->next = NULL;
        shadow_malloc(allocated); // Shadow
        return allocated->data;
    }

    struct malloc_chunk *prev = NULL;
    for (struct malloc_chunk *chunk = bins[NUM_BINS - 1]; chunk;
         chunk = chunk->next) {
        ASSERT(chunk->magic == MALLOC_FREE);

        struct malloc_chunk *allocated = NULL;
        if (chunk->capacity > size + MALLOC_FRAME_LEN) {
            allocated = split(chunk, bin_idx < 0 ? size : (1 << bin_idx));
        } else if (chunk->capacity >= size) {
            allocated = chunk;
            // Remove chunk from the linked list.
            if (prev) {
                // If it was not at the head of the list.
                prev->next = chunk->next;
            } else {
                // If it was at the head of the list.
                bins[NUM_BINS - 1] = bins[NUM_BINS - 1]->next;
            }
        }

        if (allocated) {
            allocated->magic = MALLOC_IN_USE;
            allocated->size = size;
            memset(allocated->underflow_redzone, MALLOC_REDZONE_UNDFLOW_MARKER,
                   MALLOC_REDZONE_LEN);
            memset(&allocated->data[allocated->capacity],
                   MALLOC_REDZONE_OVRFLOW_MARKER, MALLOC_REDZONE_LEN);
            allocated->next = NULL;
            shadow_malloc(allocated);
            // DBG("Created a new chunk and allocated");
            return allocated->data;
        }
        prev = chunk;
    }

    PANIC("out of memory");
}

static struct malloc_chunk *get_chunk_from_ptr(void *ptr) {
    struct malloc_chunk *chunk =
        (struct malloc_chunk *) ((uintptr_t) ptr - sizeof(struct malloc_chunk));

    // Check its magic and underflow/overflow redzones.
    ASSERT(chunk->magic == MALLOC_IN_USE);
    check_buffer_overflow(chunk);
    return chunk;
}

void free(void *ptr) {
    if (!ptr) {
        return;
    }
    struct malloc_chunk *chunk = get_chunk_from_ptr(ptr);
    if (chunk->magic == MALLOC_FREE) {
        PANIC("double-free bug!");
    }

    chunk->magic = MALLOC_FREE;

    int bin_idx = get_bin_idx_from_size(chunk->capacity);
    bin_idx = bin_idx < 0 ? NUM_BINS - 1 : bin_idx;

    struct malloc_chunk *head = bins[bin_idx];
    if (head) {
        chunk->next = head;
    }
    bins[bin_idx] = chunk;
    shadow_free(chunk);
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }

    struct malloc_chunk *chunk = get_chunk_from_ptr(ptr);
    if (chunk->capacity <= size) {
        // There's enough room. Keep using the current chunk.
        return ptr;
    }

    // There's not enough room. Allocate a new space and copy old data.
    void *new_ptr = malloc(size);
    memcpy(new_ptr, ptr, chunk->size);
    free(ptr);
    return new_ptr;
}

char *strndup(const char *s, size_t n) {
    char *new_s = malloc(n + 1);
    strncpy(new_s, s, n + 1);
    return new_s;
}

char *strdup(const char *s) {
    return strndup(s, strlen(s));
}

void malloc_init(void) {
    insert(__heap, (size_t) __heap_end - (size_t) __heap);
}

void shadow_malloc(struct malloc_chunk *chunk)
{
    // Cast the address of chunk
    DBG("%p", __shadow);
    paddr_t ptr_cur = (paddr_t)(chunk);
    DBG("next -> %u", ptr_cur);
    uint32_t reladdr = ptr_cur - (paddr_t)__heap; // Relative address
    reladdr >>= 3; // 8 to 1 mapping
    __shadow[reladdr++] = SHADOW_NEXT_PTR; // *next

    ptr_cur = (paddr_t)&(chunk->capacity);
    DBG("capacity -> %u", ptr_cur);
    reladdr = ptr_cur - (paddr_t)__heap;
    reladdr >>= 3;
    for(size_t i = 0; i < 4; i++)
    {
        __shadow[reladdr++] = SHADOW_CAPACITY; // capacity
    }

    ptr_cur = (paddr_t)&(chunk->size);
    DBG("size -> %u", ptr_cur);
    reladdr = ptr_cur - (paddr_t)__heap;
    reladdr >>= 3;
    for(size_t i = 0; i < 4; i++)
    {
        __shadow[reladdr++] = SHADOW_SIZE; // size
    }

    ptr_cur = (paddr_t)&(chunk->magic);
    DBG("magic -> %u", ptr_cur);
    reladdr = ptr_cur - (paddr_t)__heap;
    reladdr >>= 3;
    for(size_t i = 0; i < 8; i++)
    {
        __shadow[reladdr++] = SHADOW_MAGIC; // magic
    }

    ptr_cur = (paddr_t)(chunk->underflow_redzone);
    DBG("underflow redzone -> %u", ptr_cur);
    reladdr = ptr_cur - (paddr_t)__heap;
    reladdr >>= 3;
    for(size_t i = 0; i < MALLOC_REDZONE_LEN; i++)
    {
        __shadow[reladdr++] = SHADOW_UNDERFLOW_REDZONE;
    }

    ptr_cur = (paddr_t)(chunk->data);
    DBG("underflow data -> %u", ptr_cur);
    // For the time being, have no distinctions and mark everything as unaddressable
    // size_t num_bytes = sizeof(&chunk);
    // paddr_t ptr_cur = (paddr_t)chunk;
    // for(size_t i = 0; i < num_bytes/8; i++, ptr_cur+=8)
    // {
    //     // __shadow[(ptr_cur)>>3] = SHADOW_UNADDRESSABLE;
    //     uint32_t reladdr = (ptr_cur - (paddr_t)__heap);
    //     __shadow[reladdr] = SHADOW_UNADDRESSABLE;
    //     DBG("%u", reladdr);
    // }
    // DBG("Size of struct pointer - %u", sizeof(chunk->next));

    // DBG("Stack - %u", (__stack_end - __stack));

    // __shadow[(ptr_cur)>>3] = (num_bytes)%8;
    // DBG("Left out space - %d", num_bytes%8);
    // DBG("Heap is at %u - %u", __heap, __heap_end);
    // paddr_t ptr_next = chunk->next;

    // for(size_t i = 0; i < 4; i++, ptr_next++)
    // {
    //     __shadow[ptr_next>>3] = SHADOW_UNADDRESSABLE;
    // }

    // __shadow[(paddr_t)(chunk->next)>>3] = SHADOW_UNADDRESSABLE;
    // __shadow[(paddr)]


    // __shadow[(paddr_t)(chunk->capacity)] = SHADOW_CAPACITY;

}

void shadow_free(struct malloc_chunk *chunk)
{
    // Cast the address of chunk
    paddr_t ptr_cur = (paddr_t)(chunk);
    uint32_t reladdr = ptr_cur - (paddr_t)__heap; // Relative address
    reladdr >>= 3; // 8 to 1 mapping
    DBG("%u", reladdr);
    __shadow[reladdr++] = SHADOW_FREED; // *next

    ptr_cur = (paddr_t)&(chunk->capacity);
    reladdr = ptr_cur - (paddr_t)__heap;
    reladdr >>= 3;
    DBG("%u", reladdr);
    for(size_t i = 0; i < 4; i++)
    {
        __shadow[reladdr++] = SHADOW_FREED; // capacity
    }

    ptr_cur = (paddr_t)&(chunk->size);
    reladdr = ptr_cur - (paddr_t)__heap;
    reladdr >>= 3;
    DBG("%u", reladdr);
    for(size_t i = 0; i < 4; i++)
    {
        __shadow[reladdr++] = SHADOW_FREED; // size
    }

    ptr_cur = (paddr_t)&(chunk->magic);
    reladdr = ptr_cur - (paddr_t)__heap;
    reladdr >>= 3;
    DBG("%u", reladdr);
    for(size_t i = 0; i < 8; i++)
    {
        __shadow[reladdr++] = SHADOW_FREED; // magic
    }

    ptr_cur = (paddr_t)(chunk->underflow_redzone);
    reladdr = ptr_cur - (paddr_t)__heap;
    reladdr >>= 3;
    DBG("%u", reladdr);
    for(size_t i = 0; i < MALLOC_REDZONE_LEN; i++)
    {
        __shadow[reladdr++] = SHADOW_FREED;
    }

    ptr_cur = (paddr_t)(chunk->data);
    reladdr = ptr_cur - (paddr_t)__heap;
    reladdr >>= 3;
    DBG("%u", reladdr);
    for(size_t i = 0; i < (chunk->capacity + MALLOC_REDZONE_LEN); i++)
    {
        __shadow[reladdr++] = SHADOW_FREED;
    }


    // DBG("Size of chunk to be freed - %u", sizeof(*chunk));
    // for(size_t i = 0; i < sizeof(*chunk); i+=8)
    // {
    //     shadow[reladdr++] = SHADOW_FREED;
    // }
}

