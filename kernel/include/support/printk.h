#ifndef __PRINTK_H__
#define __PRINTK_H__

// Declare here instead of including header files to allow inline functions
// in header files to use these macros.
void arch_halt(void);
void backtrace(void);

void printk(const char *fmt, ...);
void print_char(char ch);
void read_kernel_log(char *buf, int buf_len);

#define KERNEL_LOG_BUFFER_MAX 4096
struct kernel_log_buffer {
    char buf[KERNEL_LOG_BUFFER_MAX];
    int head;
    int tail;
};

#define COLOR_BOLD "\e[1m"
#define COLOR_BOLD_RED "\e[1;91m"
#define COLOR_YELLOW "\e[0;33m"
#define COLOR_BLUE "\e[0;94m"
#define COLOR_MAGENTA "\e[0;95m"
#define COLOR_RESET "\e[0m"

#ifdef RELEASE_BUILD
#    define TRACE(fmt, ...)
#    define DEBUG(fmt, ...)
#else
#    define TRACE(fmt, ...) \
        printk("[kernel] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#    define DEBUG(fmt, ...) \
        printk(COLOR_MAGENTA "[kernel] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#endif

#define INFO(fmt, ...) printk(COLOR_BLUE "[kernel] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#define WARN(fmt, ...) \
    printk(COLOR_YELLOW "[kernel] WARN: " fmt COLOR_RESET "\n", ##__VA_ARGS__)

#define OOPS(fmt, ...)                                              \
    do {                                                            \
        printk(COLOR_YELLOW "[kernel] Oops: " fmt COLOR_RESET "\n", \
            ##__VA_ARGS__);                                         \
        backtrace();                                                \
    } while (0)

#define OOPS_ON_ERROR(expr)                                         \
    do {                                                            \
        error_t __err = expr;                                       \
        if (__err != OK) {                                          \
            OOPS("unexpecited error in %s()", __func__);            \
        }                                                           \
    } while (0)

#define BUG(fmt, ...)                                                \
    do {                                                             \
        printk(COLOR_BOLD_RED "[kernel] BUG: " fmt COLOR_RESET "\n", \
            ##__VA_ARGS__);                                          \
        backtrace();                                                 \
        arch_halt();                                                 \
    } while (0)

#define PANIC(fmt, ...)                                                \
    do {                                                               \
        printk(COLOR_BOLD_RED "[kernel] PANIC: " fmt COLOR_RESET "\n", \
            ##__VA_ARGS__);                                            \
        backtrace();                                                   \
        arch_halt();                                                   \
    } while (0)

#define UNIMPLEMENTED() \
    PANIC("Not yet implemented: %s:%d", __FILE__, __LINE__)

#define ASSERT(expr)                                             \
    do {                                                         \
        if (!(expr)) {                                           \
            printk(COLOR_BOLD_RED "[kernel] ASSERTION FAILURE: " \
                   #expr "\n" COLOR_RESET);                      \
            backtrace();                                         \
            arch_halt();                                         \
        }                                                        \
    } while (0)

#ifdef DEBUG_BUILD
#define DEBUG_ASSERT ASSERT
#else // DEBUG_BUILD
#define DEBUG_ASSERT(expr)
#endif

#endif
