#include <console/kprint.h>
#include <kernel/syscall.h>


void isr_handler(uint64_t *stack)
{
    uint64_t vec = stack[10];

    if (vec == 128) {
    	uint64_t num   = stack[9];
    	uint64_t arg1  = stack[4];
    	uint64_t arg2  = stack[5];
    	uint64_t arg3  = stack[6];
    	stack[9] = syscall_dispatch(num, arg1, arg2, arg3);
    	return;
    }

	kputs("exception: ");
    kputdec(vec);
    kputs("\n");
    for (;;);
}
