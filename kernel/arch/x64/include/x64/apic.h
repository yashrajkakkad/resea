#ifndef __X64_APIC_H__
#define __X64_APIC_H__

#include <types.h>
#include <memory.h>

#define APIC_REG_ID            0xfee00020
#define APIC_REG_VERSION       0xfee00030
#define APIC_REG_TPR           0xfee00080
#define APIC_REG_EOI           0xfee000b0
#define APIC_REG_LOGICAL_DEST  0xfee000d0
#define APIC_REG_DEST_FORMAT   0xfee000e0
#define APIC_REG_SPURIOUS_INT  0xfee000f0
#define APIC_REG_ICR_LOW       0xfee00300
#define APIC_REG_ICR_HIGH      0xfee00310
#define APIC_REG_LVT_TIMER     0xfee00320
#define APIC_REG_LINT0         0xfee00350
#define APIC_REG_LINT1         0xfee00360
#define APIC_REG_LVT_ERROR     0xfee00370
#define APIC_REG_TIMER_INITCNT 0xfee00380
#define APIC_REG_TIMER_CURRENT 0xfee00390
#define APIC_REG_TIMER_DIV     0xfee003e0
#define APIC_TIMER_VECTOR 32

void x64_apic_init(void);
void x64_apic_timer_init(void);
void x64_ack_interrupt(void);

#endif
