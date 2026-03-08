#ifndef STACK_H
#define STACK_H

#include <stdint.h>

/* Number of stack frames to walk */
#define STACK_MAX_FRAMES    32

/* ─── Stack frame structure (x86 standard frame layout) ───────────────────── */
typedef struct stack_frame {
    struct stack_frame *ebp_prev;   /* Saved EBP (caller's frame) */
    uint32_t            eip_ret;    /* Return address             */
} stack_frame_t;

/*
 * print_stack_trace()
 *   Walk the call stack from current EBP and print each frame in a
 *   human-friendly way:
 *
 *   ┌─────────────────────────────────────────┐
 *   │  Kernel Stack Trace                     │
 *   ├──────┬──────────────┬───────────────────┤
 *   │ #00  │ EBP 0x...    │ RET 0x...         │
 *   │ #01  │ EBP 0x...    │ RET 0x...         │
 *   └──────┴──────────────┴───────────────────┘
 */
void print_stack_trace(void);

/*
 * print_stack_dump(esp, size)
 *   Raw hex dump of [size] bytes starting at [esp].
 */
void print_stack_dump(uint32_t esp, uint32_t size);

/*
 * print_registers()
 *   Capture and display current CPU register state.
 */
void print_registers(void);

#endif /* STACK_H */
