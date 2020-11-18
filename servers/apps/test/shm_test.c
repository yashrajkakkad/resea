#include <string.h>
#include "resea/shm.h"
#include "test.h"

void shm_test(void) {
    int fd = create(2);
    TEST_ASSERT(fd >= 0);

    struct shm_t *shm_obj;
    shm_obj = stat(fd);
    // test shm_obj is  returned
    TEST_ASSERT(shm_obj != NULL);
    // test shm_obj has paddr allocated
    TEST_ASSERT(shm_obj->paddr);
    // test stat of random id is NULL
    TEST_ASSERT(stat(11) != NULL);
    vaddr_t vaddr = map(fd);
    TEST_ASSERT(vaddr > -1);

    TEST_ASSERT(IS_OK(unmap(vaddr)));

    TEST_ASSERT(IS_OK(close(fd)));

    TEST_ASSERT(stat(fd) == NULL);
}