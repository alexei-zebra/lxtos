#include <arch/x86_64/idt.h>


extern void idt_load(struct idt_ptr*);
extern void irq1();


static struct idt_entry idt[256];
static struct idt_ptr idtp;


void idt_set(int vec, void *isr, uint8_t flags)
{
    uint64_t addr = (uint64_t)isr;

    idt[vec].offset_low  = addr & 0xFFFF;
    idt[vec].selector    = 0x08;
    idt[vec].ist         = 0;
    idt[vec].type_attr   = flags;
    idt[vec].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (addr >> 32);
    idt[vec].zero        = 0;
}

void idt_init(void)
{
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)&idt;

    extern void isr0();
    extern void isr128();

    idt_set(0, isr0, 0x8E);      // exception
    idt_set(128, isr128, 0xEE);  // user syscall (DPL=3)
    idt_set(33, irq1, 0x8E); // keyboard

    idt_load(&idtp);
}
