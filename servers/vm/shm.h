#ifndef __SHM_H__
#define __SHM_H__

#include <types.h>
struct shm
{
    bool inuse;
    int shm_id;
    paddr_t paddr;
    size_t len;
};

#define NUM_SHARED_MEMS_MAX 32
static struct shm shared_mems[NUM_SHARED_MEMS_MAX];

int create(size_t size);
vaddr_t map(int shm_id);
int unmap(int shm_id);
int close(int shm_id);
#endif