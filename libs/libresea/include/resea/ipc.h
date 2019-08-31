#ifndef __RESEA_IPC_H__
#define __RESEA_IPC_H__

#include <resea/types.h>

/* header */
#define MSG_INLINE_LEN_OFFSET 0
#define MSG_NUM_PAGES_OFFSET 12
#define MSG_NUM_CHANNELS_OFFSET 14
#define MSG_LABEL_OFFSET 16
#define INLINE_PAYLOAD_LEN(header) (((header) >> MSG_INLINE_LEN_OFFSET) & 0x7ff)
#define PAGE_PAYLOAD_NUM(header) (((header) >> MSG_NUM_PAGES_OFFSET) & 0x3)
#define PAGE_PAYLOAD_ADDR(page) ((page) &0xfffffffffffff000ull)
#define MSG_LABEL(header) (((header) >> MSG_LABEL_OFFSET) & 0xffff)
#define CHANNELS_PAYLOAD_NUM(header) \
    (((header) >> MSG_NUM_CHANNELS_OFFSET) & 0x3)
#define INTERFACE_ID(header) (MSG_LABEL(header) >> 8)
#define INLINE_PAYLOAD_LEN_MAX 2047
#define ERROR_TO_HEADER(error) ((uint32_t)(error) << MSG_LABEL_OFFSET)

// Syscall ops.
#define IPC_SEND (1ull << 8)
#define IPC_RECV (1ull << 9)
#define IPC_REPLY (1ull << 10)

#define PAGE_PAYLOAD(addr, exp, type) (addr | (exp << 3) | (type << 5))
#define PAGE_TYPE_MOVE   1
#define PAGE_TYPE_SHARED 2

#define SYSCALL_IPC 0
#define SYSCALL_OPEN 1
#define SYSCALL_CLOSE 2
#define SYSCALL_LINK 3
#define SYSCALL_TRANSFER 4

typedef uint32_t header_t;
typedef uintptr_t page_t;

struct message {
    uint32_t header;
    cid_t from;
    cid_t channels[4];
    page_t pages[4];
    uint8_t data[INLINE_PAYLOAD_LEN_MAX];
} PACKED;

struct thread_info {
    uintmax_t arg;
    struct message ipc_buffer;
} PACKED;

struct message *get_ipc_buffer(void);
void copy_to_ipc_buffer(const struct message *m);
void copy_from_ipc_buffer(struct message *buf);
int sys_open(void);
error_t sys_close(cid_t ch);
error_t sys_link(cid_t ch1, cid_t ch2);
error_t sys_transfer(cid_t src, cid_t dst);
error_t sys_ipc(cid_t ch, uint32_t ops);

error_t open(cid_t *ch);
error_t close(cid_t ch);
error_t link(cid_t ch1, cid_t ch2);
error_t transfer(cid_t src, cid_t dst);
error_t ipc_recv(cid_t ch, struct message *r);
error_t ipc_call(cid_t ch, struct message *m, struct message *r);
error_t ipc_replyrecv(cid_t ch, struct message *m, struct message *r);

#endif