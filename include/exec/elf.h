#pragma once
#include <stdint.h>
#include <mm/vmm.h>

#define ELF_MAGIC  0x464C457FU  /* \x7FELF */
#define ET_EXEC    2
#define EM_X86_64  0x3E
#define PT_LOAD    1
#define PF_X       1
#define PF_W       2
#define PF_R       4

typedef struct __attribute__((packed)) {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_hdr_t;

typedef struct __attribute__((packed)) {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr_t;

int elf_load(pml4_t pml4, const uint8_t *data,
                  uint64_t size, uint64_t *out_entry);

int elf_exec(const char *path);
int elf_exec_argv(const char *path, const char **uargv);