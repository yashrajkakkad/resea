#include <arch.h>
#include <task.h>
#include <printk.h>
#include <string.h>
#include "vm.h"

static uint64_t *traverse_page_table(uint64_t pml4, vaddr_t vaddr,
                                     paddr_t kpage, uint64_t attrs) {
    ASSERT(vaddr < KERNEL_BASE_ADDR);
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
    ASSERT(IS_ALIGNED(kpage, PAGE_SIZE));

    uint64_t *table = from_paddr(pml4);
    for (int level = 4; level > 1; level--) {
        int index = NTH_LEVEL_INDEX(level, vaddr);
        if (!table[index]) {
            if (!attrs) {
                return NULL;
            }

            /* The PDPT, PD or PT is not allocated. */
            if (!kpage) {
                return NULL;
            }

            memset(from_paddr(kpage), 0, PAGE_SIZE);
            table[index] = kpage;
            kpage = 0;
        }

        // Update attributes if given.
        table[index] = table[index] | attrs;

        // Go into the next level paging table.
        table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
    }

    return &table[NTH_LEVEL_INDEX(1, vaddr)];
}

error_t arch_vm_map(struct task *task, vaddr_t vaddr, paddr_t paddr, paddr_t kpage,
                unsigned flags) {
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));
    uint64_t attrs = (1 << 2) | 1 /* user, present */;
    attrs |= (flags & MAP_W) ? X64_PAGE_WRITABLE : 0;

    uint64_t *entry = traverse_page_table(task->arch.pml4, vaddr, kpage, attrs);
    if (!entry) {
        return (kpage) ? ERR_TRY_AGAIN : ERR_EMPTY;
    }

    *entry = paddr | attrs;
    asm_invlpg(vaddr);
    return OK;
}

error_t arch_vm_unmap(struct task *task, vaddr_t vaddr) {
    uint64_t *entry = traverse_page_table(task->arch.pml4, vaddr, 0, 0);
    if (!entry) {
        return ERR_NOT_FOUND;
    }

    *entry = 0;
    asm_invlpg(vaddr);
    return OK;
}

paddr_t vm_resolve(struct task *task, vaddr_t vaddr) {
    uint64_t *entry = traverse_page_table(task->arch.pml4, vaddr, 0, 0);
    return (entry) ? ENTRY_PADDR(*entry) : 0;
}
