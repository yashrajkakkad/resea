#ifndef __SHM_H__
#define __SHM_H__

#include <types.h>

struct shm_t {
    bool inuse;
    int shm_id;
    paddr_t paddr;
    size_t len;
};
int shm_create(size_t size);
vaddr_t shm_map(int shm_id);
error_t shm_unmap(vaddr_t vaddr);
int shm_close(int shm_id);
struct shm_t* shm_stat(int shm_id);
#endif