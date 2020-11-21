#include <string.h>
#include "ipc.h"
#include "shm.h"

int shm_create(size_t size, paddr_t paddr) {
    int i = 0;
    for (; i < NUM_SHARED_MEMS_MAX && shared_mems[i].inuse; i++)
        ;

    if (i == NUM_SHARED_MEMS_MAX)
        return -1;

    shared_mems[i].inuse = true;
    shared_mems[i].shm_id = i;
    shared_mems[i].len = size;
    // // ipc vm to allocate
    // task_t vm = ipc_lookup("vm");
    // struct message m;
    // m.type = VM_ALLOC_PAGES_MSG;
    // m.vm_alloc_pages.num_pages = size;
    // m.vm_alloc_pages.paddr = NULL;
    // ASSERT_OK(ipc_call(vm, &m));
    shared_mems[i].paddr = paddr;
    return i;
}

int shm_close(int shm_id) {
    int i = 0;
    // unallocate p_addr
    for (; i < NUM_SHARED_MEMS_MAX; i++) {
        if (shared_mems[i].shm_id == shm_id)
            bzero(&shared_mems[i], sizeof(struct shm_t));
        return 0;
    }
    return -1;
}

struct shm_t* shm_stat(int shm_id) {
    int i = 0;
    for (; i < NUM_SHARED_MEMS_MAX; i++) {
        if (shared_mems[i].shm_id == shm_id)
            return &shared_mems[i];
    }
    return NULL;
}
vaddr_t shm_map(task_t tid, int shm_id) {
    // struct shm_t* s = stat(shm_id);
    // vaddr_t vaddr = alloc_virt_pages(tid, s->len);
    // not sure abt flags and overwrite
    // map_page(tid, vaddr, s->paddr, 0, true);

    return 0;
}
