#include <resea/task.h>
#include <string.h>
#include "pages.h"
#include "shm.h"
#include "task.h"

int create(size_t size) {
    int i = 0;
    for (; i < NUM_SHARED_MEMS_MAX && shared_mems[i].inuse; i++)
        ;

    if (i == NUM_SHARED_MEMS_MAX)
        return -1;

    shared_mems[i].inuse = true;
    shared_mems[i].shm_id = i;
    shared_mems[i].len = size;
    shared_mems[i].paddr = pages_alloc(size);
    return i;
}

int close(int shm_id) {
    int i = 0;
    // unallocate p_addr
    for (; i < NUM_SHARED_MEMS_MAX; i++) {
        if (shared_mems[i].shm_id == shm_id)
            bzero(&shared_mems[i], sizeof(struct shm_t));
        return 0;
    }
    return -1;
}

struct shm_t* stat(int shm_id) {
    int i = 0;
    for (; i < NUM_SHARED_MEMS_MAX; i++) {
        if (shared_mems[i].shm_id == shm_id)
            return &shared_mems[i];
    }
    return NULL;
}
vaddr_t map(int shm_id) {
    struct shm_t* s = stat(shm_id);
    vaddr_t vaddr = alloc_virt_pages(CURRENT, s->len);
    // not sure abt flags and overwrite
    map_page(CURRENT, vaddr, s->paddr, 0, true);

    return vaddr;
}

int unmap(vaddr_t vaddr) {
    return vm_unmap(CURRENT, vaddr)
}