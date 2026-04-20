#!/usr/bin/env python3
"""Build system for NocturneOS."""

import glob
import os
import sys

CC = "gcc"
LD = "ld"
AS = "nasm"

CFLAGS_LIST = [
    "-m64",
    "-ffreestanding",
    "-fno-stack-protector",
    "-fno-pic",
    "-fno-pie",
    "-mno-red-zone",
    "-mno-mmx",
    "-mno-sse",
    "-mno-sse2",
    "-nostdlib",
    "-Wall",
    "-Wextra",
    "-O2"
]
LDFLAGS_LIST = [
    "-m", "elf_x86_64",
    "-nostdlib",
    "-static",
    "-T", "linker.ld"
]
ASFLAGS_LIST = [
    "-f", "elf64"
]

KERN_CFLAGS_LIST = CFLAGS_LIST + ["-Iinclude", "-mcmodel=kernel"]
KERN_LDFLAGS_LIST = LDFLAGS_LIST + ["-z", "max-page-size=0x1000"]
KERN_ASFLAGS_LIST = ASFLAGS_LIST

USER_CFLAGS_LIST = CFLAGS_LIST + ["-Iuserspace/include", "-Iinclude"]
USER_LDFLAGS_LIST = LDFLAGS_LIST
USER_ASFLAGS_LIST = ASFLAGS_LIST

KERN_CFLAGS = ' '.join(KERN_CFLAGS_LIST)
KERN_ASFLAGS = ' '.join(KERN_ASFLAGS_LIST)
KERN_LDFLAGS = ' '.join(KERN_LDFLAGS_LIST)

USER_CFLAGS = ' '.join(USER_CFLAGS_LIST)
USER_LDFLAGS = ' '.join(USER_LDFLAGS_LIST)
USER_ASFLAGS = ' '.join(USER_ASFLAGS_LIST)

OS_NAME = "NocturneOS"
KERNEL = "kernel.elf"
ISO = f"{OS_NAME}.iso"
DISK_IMG = "disk.img"

LIMINE_REPO = "https://codeberg.org/Limine/Limine.git"
LIMINE_REPO_ARG = "--branch=v11.x-binary --depth=1"
LIMINE_DIR = "limine"


USER_COMMON = [
    "userspace/src/crt0.asm",
    "userspace/src/ulib/io.c",
    "userspace/src/ulib/string.c",
]


def sh(cmd: str, no_exit=False):
    print(cmd)
    ret = os.system(cmd)
    if ret != 0:
        print(f"Error: '{cmd}' finished with code {ret}")
        return ret if no_exit else sys.exit(1)


def newer(src: str, dst: str):
    if not os.path.exists(dst):
        return True
    return os.path.getmtime(src) > os.path.getmtime(dst)


def find_files(directory: str, extension: str):
    return glob.glob(f"{directory}/**/*{extension}", recursive=True)


def build_user_binary(out_path: str, sources: list[str]):
    os.makedirs("build/ubin", exist_ok=True)
    objs = []
    for src in sources:
        safe = src.replace("/", "_").replace(".", "_")
        obj = f"build/ubin/{safe}.o"
        objs.append(obj)
        if not newer(src, obj):
            continue
        if src.endswith(".c"):
            sh(f"{CC} {USER_CFLAGS} -c {src} -o {obj}")
        elif src.endswith(".asm"):
            sh(f"{AS} {USER_ASFLAGS} {src} -o {obj}")
    sh(f"{LD} {USER_LDFLAGS} -o {out_path} {' '.join(objs)}")


def build_userspace():
    os.makedirs("initramfs/bin", exist_ok=True)
    build_user_binary(
        "initramfs/bin/shell",
        USER_COMMON + ["userspace/src/shell.c"],
    )
    build_user_binary(
        "initramfs/bin/hello",
        USER_COMMON + ["userspace/src/hello.c"],
    )


def build_initramfs():
    os.makedirs("build", exist_ok=True)
    os.makedirs("initramfs/bin", exist_ok=True)
    os.makedirs("initramfs/etc", exist_ok=True)
    with open("initramfs/etc/osname", "w") as f:
        f.write(f"{OS_NAME}\n")
    build_userspace()
    sh("cd initramfs && find . | cpio -o -H newc > ../build/initramfs.cpio")
    sh(f"{LD} -r -b binary build/initramfs.cpio -o build/initramfs_bin.o")


def get_sources():
    jobs = []
    for src in find_files("src", ".c"):
        rel = os.path.relpath(src, "src")
        obj = os.path.join("build", rel[:-2] + ".o")
        jobs.append(("c", src, obj))
    for src in find_files("src", ".asm"):
        rel = os.path.relpath(src, "src")
        obj = os.path.join("build", rel[:-4] + "_asm.o")
        jobs.append(("asm", src, obj))
    return jobs


def compile_kernel_sources():
    jobs = get_sources()
    obj_files = []
    for kind, src, obj in jobs:
        obj_files.append(obj)
        os.makedirs(os.path.dirname(obj), exist_ok=True)
        if not newer(src, obj):
            continue
        if kind == "c":
            sh(f"{CC} {KERN_CFLAGS} -c {src} -o {obj}")
        elif kind == "asm":
            sh(f"{AS} {KERN_ASFLAGS} {src} -o {obj}")
    return obj_files


def build_kernel():
    build_initramfs()
    obj_files = compile_kernel_sources()
    obj_files.append("build/initramfs_bin.o")
    sh(f"{LD} {KERN_LDFLAGS} -o {KERNEL} {' '.join(obj_files)}")


def clone_limine():
    if not os.path.exists(LIMINE_DIR):
        sh(f"git clone --branch=v11.x-binary --depth=1 {LIMINE_REPO} {LIMINE_DIR}")


def build_iso():
    build_kernel()
    clone_limine()
    os.makedirs("iso_root/boot/limine", exist_ok=True)
    os.makedirs("iso_root/EFI/BOOT", exist_ok=True)
    sh(f"cp {KERNEL} iso_root/boot/")
    sh("cp limine.conf iso_root/boot/limine/")
    for f in ["limine-bios.sys", "limine-bios-cd.bin", "limine-uefi-cd.bin"]:
        sh(f"cp {LIMINE_DIR}/{f} iso_root/boot/limine/")
    sh(f"cp {LIMINE_DIR}/BOOTX64.EFI iso_root/EFI/BOOT/")
    sh(f"cp {LIMINE_DIR}/BOOTIA32.EFI iso_root/EFI/BOOT/")
    sh(f"xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin\
        -no-emul-boot -boot-load-size 4 -boot-info-table\
        --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part\
        --efi-boot-image --protective-msdos-label iso_root -o {ISO}")


def create_disk():
    if not os.path.exists(DISK_IMG):
        sh(f"dd if=/dev/zero of={DISK_IMG} bs=1M count=64")
        sh(f"mkfs.ext2 -b 1024 {DISK_IMG}")


def run_qemu():
    build_iso()
    create_disk()
    sh(f"qemu-system-x86_64 -cdrom {ISO}\
        -drive file={DISK_IMG},format=raw,if=ide,index=1\
        -m 128M -vga std -no-reboot -no-shutdown")


def populate_disk():
    if not os.path.exists(DISK_IMG):
        sh(f"dd if=/dev/zero of={DISK_IMG} bs=1M count=64")
        sh(f"mkfs.ext2 -b 1024 {DISK_IMG}")
    sh(f"mkdir -p /tmp/{OS_NAME}_mnt")
    sh(f"sudo mount -o loop {DISK_IMG} /tmp/{OS_NAME}_mnt")
    sh(f"sudo mkdir -p /tmp/{OS_NAME}_mnt/etc")
    sh(f"echo 'hello from ext2' | sudo tee /tmp/{OS_NAME}_mnt/etc/hello.txt")
    sh(f"echo '{OS_NAME}' | sudo tee /tmp/{OS_NAME}_mnt/etc/hostname")
    sh(f"sudo umount /tmp/{OS_NAME}_mnt")


def clean():
    sh("rm -rf build iso_root limine")
    sh(f"rm -f {KERNEL} {ISO}")


def clean_disk():
    sh(f"rm -f {DISK_IMG}")


TARGETS = {
    "all": build_kernel,
    "iso": build_iso,
    "run": run_qemu,
    "clean": clean,
    "clean-disk": clean_disk,
    "disk-populate": populate_disk,
}

if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "all"
    if target not in TARGETS:
        print(f"Unknown target: '{target}'")
        print(f"Available: {', '.join(TARGETS)}")
        sys.exit(1)
    TARGETS[target]()
