#pragma once
#include <stdint.h>
#include <uapi/syscall_nr.h>

static inline void sys_puts(const char *s)
{
    __asm__ volatile("mov $1,%%rax; mov %0,%%rdi; int $0x80"
        :: "r"((uint64_t)s) : "rax","rdi");
}

static inline void sys_gets(char *buf, uint64_t max)
{
    __asm__ volatile("mov $4,%%rax; mov %0,%%rdi; mov %1,%%rsi; int $0x80"
        :: "r"((uint64_t)buf), "r"(max) : "rax","rdi","rsi");
}

static inline void sys_exit(void)
{
    __asm__ volatile("mov $3,%rax; int $0x80");
    for (;;);
}

static inline void sys_clear(void)
{
    __asm__ volatile("mov $5, %%rax; int $0x80" ::: "rax");
}


static inline int64_t sys_readdir(const char *path, uint32_t idx, char *out)
{
    uint64_t ret;
    __asm__ volatile("mov $6,%%rax; int $0x80"
        : "=a"(ret)
        : "D"((uint64_t)path), "S"((uint64_t)idx), "d"((uint64_t)out)
        : "memory");
    return (int64_t)ret;
}

static inline int64_t sys_mkdir(const char *path)
{
    uint64_t ret;
    __asm__ volatile("mov $7,%%rax; int $0x80"
        : "=a"(ret) : "D"((uint64_t)path) : "memory");
    return (int64_t)ret;
}

static inline int64_t sys_mkfile(const char *path)
{
    uint64_t ret;
    __asm__ volatile("mov $8,%%rax; int $0x80"
        : "=a"(ret) : "D"((uint64_t)path) : "memory");
    return (int64_t)ret;
}

static inline int64_t sys_rm(const char *path)
{
    uint64_t ret;
    __asm__ volatile("mov $9,%%rax; int $0x80"
        : "=a"(ret) : "D"((uint64_t)path) : "memory");
    return (int64_t)ret;
}

static inline int64_t sys_read(const char *path, void *buf, uint64_t sz)
{
    uint64_t ret;
    __asm__ volatile("mov $10,%%rax; int $0x80"
        : "=a"(ret)
        : "D"((uint64_t)path), "S"((uint64_t)buf), "d"(sz)
        : "memory");
    return (int64_t)ret;
}

static inline int64_t sys_write(const char *path, const void *buf, uint64_t sz)
{
    uint64_t ret;
    __asm__ volatile("mov $11,%%rax; int $0x80"
        : "=a"(ret)
        : "D"((uint64_t)path), "S"((uint64_t)buf), "d"(sz)
        : "memory");
    return (int64_t)ret;
}

static inline int64_t sys_exec(const char *path, const char **argv)
{
    uint64_t ret;
    __asm__ volatile(
        "mov $12, %%rax; int $0x80"
        : "=a"(ret)
        : "D"((uint64_t)path), "S"((uint64_t)argv)
        : "memory"
    );
    return (int64_t)ret;
}

static inline uint64_t sys_fsize(const char *path)
{
    uint64_t ret;
    __asm__ volatile(
        "mov $13, %%rax; int $0x80"
        : "=a"(ret)
        : "D"((uint64_t)path)
        : "memory"
    );
    return ret;
}
