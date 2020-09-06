#include <types.h>
#include <printk.h>
#include "asm.h"
#include "peripherals.h"

void uart_send(unsigned int c) {
    while (mmio_read(UART0_FR) & (1 << 5)) {}
    mmio_write(UART0_DR, c);
}

char uart_getc() {
    while (mmio_read(UART0_FR) & (1 << 4)) {}
    return mmio_read(UART0_DR);
}



/* a properly aligned buffer */
extern volatile unsigned int mbox[36];

#define MMIO_BASE       0x3F000000 + 0xffff000000000000
#define MBOX_REQUEST    0

/* channels */
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

/* tags */
#define MBOX_TAG_GETSERIAL      0x10004
#define MBOX_TAG_SETCLKRATE     0x38002
#define MBOX_TAG_LAST           0
/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[36];

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

static inline void delay(unsigned clocks) {
    while (clocks-- > 0) {
        __asm__ __volatile__("nop");
    }
}

/// Performs a mailbox call.
void mbox_call(uint8_t ch, const uint8_t *mbox) {
    ASSERT(IS_ALIGNED((vaddr_t) mbox, 0x10));

    while (*MBOX_STATUS & MBOX_FULL) {
        delay(1);
    }

    unsigned int value =
        (((vaddr_t) mbox) & ~0xf) // The address of the mailbox to be sent.
        | (ch & 0xf)              // Channel.
        ;
    *MBOX_WRITE = value;
    while (1) {
        while (*MBOX_STATUS & MBOX_EMPTY) {
            delay(1);
        }

        if (value == *MBOX_READ) {
            break;
        }
    }
}

void uart_init(void) {
    // Set up the clock.
    mbox[0] = 9*4;
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_SETCLKRATE; // set clock rate
    mbox[3] = 12;
    mbox[4] = 8;
    mbox[5] = 2;           // UART clock
    mbox[6] = 4000000;     // 4Mhz
    mbox[7] = 0;           // clear turbo
    mbox[8] = MBOX_TAG_LAST;
    mbox_call(MBOX_CH_PROP, (void *) mbox);

    // Initialize GPIO pins #14 and #15 as alternative function 0 (UART0).
    mmio_write(GPIO_FSEL1,
        (mmio_read(GPIO_FSEL1) & ~((0b111 << 12)|(0b111 << 15)))
        | (0b100 << 12) | (0b100 << 15));

    // See "GPIO Pull-up/down Clock Registers (GPPUDCLKn)" in "BCM2835 ARM Peripherals"
    // for more details on this initializaiton sequence.
    mmio_write(GPIO_PUD, 0);
    delay(150);
    mmio_write(GPIO_PUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    mmio_write(GPIO_PUDCLK0, 0);

    mmio_write(UART0_CR, 0);            // Disable UART.
    mmio_write(UART0_ICR, 0x7ff);       // Disable interrupts from UART.
    mmio_write(UART0_IBRD, 2);          // baud rate = 115200
    mmio_write(UART0_FBRD, 11);         //
    mmio_write(UART0_LCRH, 0b11 << 5);  // 8n1
    mmio_write(UART0_CR, 0x301);        // Enable RX, TX, and UART0.
}

void arch_printchar(char ch) {
    uart_send(ch);
}

char kdebug_readchar(void) {
    return '\0'; // TODO:
}

bool kdebug_is_readable(void) {
    // TODO: Not yet implemented.
    return false;
}

void watchdog_init(void) {
	mmio_write(PM_WDOG, PM_PASSWORD | 10 << 16 /* 10 seconds */);
	mmio_write(PM_RSTC, PM_PASSWORD | PM_WDOG_FULL_RESET);
}

void arm64_timer_reload(void) {
    uint64_t hz = ARM64_MRS(cntfrq_el0);
    ASSERT(hz >= 1000);
    hz /= 1000;
    ARM64_MSR(cntv_tval_el0, hz);
}

static void timer_init(void) {
    arm64_timer_reload();
    ARM64_MSR(cntv_ctl_el0, 1ull);
    mmio_write(CORE0_TIMER_IRQCNTL, 1 << 3 /* Enable nCNTVIRQ IRQ */);
}

void arm64_peripherals_init(void) {
    uart_init();
    watchdog_init();
    timer_init();
}

void arch_enable_irq(unsigned irq) {
    // TODO:
}

void arch_disable_irq(unsigned irq) {
    // TODO:
}
