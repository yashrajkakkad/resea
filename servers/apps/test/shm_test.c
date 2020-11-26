#include <resea/shm.h>
#include <string.h>
#include "test.h"

void shm_test(void) {
    int fd = shm_create(2);
    int fd2 = shm_create(4);
    TEST_ASSERT(fd >= -1);

    struct shm_t *shm_obj;
    shm_obj = shm_stat(0);
    // test shm_obj is  returned
    TEST_ASSERT(shm_obj != NULL);
    // test stat of random id is NULL
    TEST_ASSERT(shm_stat(11) == NULL);
    // vaddr_t vaddr = shm_map(fd);
    // TEST_ASSERT(vaddr > -1);

    // TEST_ASSERT(IS_OK(shm_unmap(vaddr)));

    TEST_ASSERT(IS_OK(shm_close(fd)));

    TEST_ASSERT(shm_stat(fd) == NULL);
}