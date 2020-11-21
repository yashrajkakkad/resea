#include <resea/ipc.h>
#include <resea/shm.h>
#include <resea/syscall.h>

int shm_create(size_t size) {
    // // ipc vm to allocate
    task_t vm = ipc_lookup("vm");
    struct message m;
    m.type = VM_ALLOC_PAGES_MSG;
    m.vm_alloc_pages.num_pages = size;
    m.vm_alloc_pages.paddr = NULL;
    ASSERT_OK(ipc_call(vm, &m));
    paddr_t paddr = m.vm_alloc_pages_reply.paddr;
    return sys_shm_create(size, paddr);
}
vaddr_t shm_map(int shm_id) {
    return sys_shm_map(shm_id);
}
int shm_close(int shm_id) {
    return sys_shm_close(shm_id);
}
struct shm_t* shm_stat(int shm_id) {
    return (struct shm_t*) sys_shm_stat(shm_id);
}
error_t shm_unmap(vaddr_t vaddr) {
    return sys_shm_unmap(vaddr);
}
