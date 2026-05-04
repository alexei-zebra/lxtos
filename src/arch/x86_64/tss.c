#include <arch/x86_64/gdt.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

#define STACK_SIZE 0x4000


static uint8_t *kernel_stack;
extern struct tss kernel_tss;

void tss_init(void)
{
    uint64_t phys = pmm_alloc_n(4);
    kernel_stack = (uint8_t *)(phys + vmm_hhdm_offset());

    kernel_tss.rsp0 = (uint64_t)(kernel_stack + STACK_SIZE);
}
