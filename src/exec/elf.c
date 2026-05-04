#include <exec/elf.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <lib/kmalloc.h>
#include <lib/kstring.h>
#include <console/kprint.h>
#include <fs/vfs.h>
#include <arch/x86_64/usermode.h>

#define USER_STACK_TOP  0x7FFFFFFFE000ULL
#define USER_STACK_PAGES 16


int elf_load(pml4_t pml4, const uint8_t *data,
             uint64_t size, uint64_t *out_entry)
{
    if (size < sizeof(elf64_hdr_t)) { kputs("elf: too small\n"); return -1; }

    const elf64_hdr_t *hdr = (const elf64_hdr_t *)data;

    if (*(const uint32_t *)hdr->e_ident != ELF_MAGIC) {
        kputs("elf: bad magic\n"); return -1;
    }
    if (hdr->e_ident[4] != 2) { kputs("elf: not 64-bit\n"); return -1; }
    if (hdr->e_machine   != EM_X86_64) { kputs("elf: not x86-64\n"); return -1; }
    if (hdr->e_type      != ET_EXEC)   { kputs("elf: not executable\n"); return -1; }

    uint64_t hhdm = vmm_hhdm_offset();

    for (uint16_t i = 0; i < hdr->e_phnum; i++) {
        const elf64_phdr_t *ph =
            (const elf64_phdr_t *)(data + hdr->e_phoff
                                   + (uint64_t)i * hdr->e_phentsize);
        if (ph->p_type != PT_LOAD || ph->p_memsz == 0) continue;

        uint64_t vaddr     = ph->p_vaddr;
        uint64_t page_base = vaddr & ~(uint64_t)(PAGE_SIZE - 1);
        uint64_t page_end  = (vaddr + ph->p_memsz + PAGE_SIZE - 1)
                             & ~(uint64_t)(PAGE_SIZE - 1);

        uint64_t flags = PAGE_PRESENT | PAGE_USER;
        if (ph->p_flags & PF_W) flags |= PAGE_WRITE;

		// maping
        for (uint64_t va = page_base; va < page_end; va += PAGE_SIZE) {
            uint64_t phys = pmm_alloc();
            if (!phys) { kputs("elf: oom\n"); return -1; }
            vmm_map(pml4, va, phys, flags);

            uint8_t *dst = (uint8_t *)(phys + hhdm);
            kmemset(dst, 0, PAGE_SIZE);

            for (uint64_t b = 0; b < PAGE_SIZE; b++) {
                uint64_t byte_va  = va + b;
                if (byte_va < vaddr) continue;
                uint64_t seg_off  = byte_va - vaddr;
                if (seg_off >= ph->p_filesz) break;
                dst[b] = data[ph->p_offset + seg_off];
            }
        }
    }

    *out_entry = hdr->e_entry;
    return 0;
}

int elf_exec(const char *path) {
    const char *argv[] = { path, NULL };
    return elf_exec_argv(path, argv);
}

int elf_exec_argv(const char *path, const char **uargv)
{
    int argc = 0;
    if (uargv) {
        while (uargv[argc]) argc++;
    }
    const char *fallback[] = { path, NULL };
    if (argc == 0) { uargv = fallback; argc = 1; }

    vfs_node_t *node = vfs_resolve(path);
    if (!node || (node->flags & VFS_FLAG_DIR)) {
        kputs("elf_exec: not found: "); kputs(path); kputs("\n");
        return -1;
    }

    uint64_t fsize = node->size;
    if (fsize == 0) { kputs("elf_exec: empty file\n"); return -1; }

    uint8_t *buf = kmalloc(fsize);
    if (!buf) { kputs("elf_exec: oom\n"); return -1; }
    vnode_read(node, buf, 0, fsize);

    pml4_t pml4 = vmm_new_space();
    uint64_t hhdm = vmm_hhdm_offset();
    uint64_t stack_bottom = USER_STACK_TOP - (uint64_t)USER_STACK_PAGES * PAGE_SIZE;
    uint64_t stack_phys[USER_STACK_PAGES];

    for (uint64_t pg = 0; pg < USER_STACK_PAGES; pg++) {
        uint64_t phys = pmm_alloc();
        if (!phys) { kfree(buf); return -1; }
        vmm_map(pml4, stack_bottom + pg * PAGE_SIZE, phys,
                PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        kmemset((void *)(phys + hhdm), 0, PAGE_SIZE);
        stack_phys[pg] = phys;
    }

    uint64_t entry = 0;
    if (elf_load(pml4, buf, fsize, &entry) != 0) { kfree(buf); return -1; }
    kfree(buf);

    #define VIRT_TO_KERN(va) \
        ((void *)(stack_phys[((va) - stack_bottom) / PAGE_SIZE] + hhdm \
                  + ((va) - stack_bottom) % PAGE_SIZE))

    uint64_t sp = USER_STACK_TOP;
    uint64_t arg_ptrs[16];


    for (int i = argc - 1; i >= 0; i--) {
        int len = kstrlen(uargv[i]) + 1;
        sp -= len;
        kmemcpy(VIRT_TO_KERN(sp), uargv[i], len);
        arg_ptrs[i] = sp;
    }

    sp &= ~7ULL;

    // NULL
    sp -= 8;
    *(uint64_t *)VIRT_TO_KERN(sp) = 0;

    // argv[]
    for (int i = argc - 1; i >= 0; i--) {
        sp -= 8;
        *(uint64_t *)VIRT_TO_KERN(sp) = arg_ptrs[i];
    }

    // argc
    sp -= 8;
    *(uint64_t *)VIRT_TO_KERN(sp) = (uint64_t)argc;

    #undef VIRT_TO_KERN

    vmm_switch(pml4);
    jump_usermode(entry, sp);
    return 0;
}