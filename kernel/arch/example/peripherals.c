#include <types.h>
#include <printk.h>

void arch_printchar(char ch) {
}

int kdebug_readchar(void) {
    return -1;
}

bool kdebug_is_readable(void) {
    return false;
}
