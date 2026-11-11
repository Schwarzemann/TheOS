# TheOS

A small x86 operating system built from scratch.

## Building and running

Requires `nasm`, `gcc` (with 32-bit support, `-m32`), `ld`, `objcopy`,
`python3` and `qemu-system-i386`.

```sh
make all              # build and leave it as it is without running it
make run              # build and boot in QEMU with a graphical window
make run-headless      # build and boot with serial output on stdout, no display
make reset-fs         # discard files written inside the OS, start again from fs/
make clean
```

Files written inside the OS are stored in `build/TheOS.img` and survive kernel
rebuilds. Changing anything under `fs/` or the TOFS tools replaces them with a
fresh filesystem, as do `make reset-fs` and `make clean`.

## Known limitations / what's not here

These will be added in the future:

- **No process isolation between user tasks.** Kernel memory is
  supervisor-only, page 0 is unmapped, syscalls check user pointers, and a
  ring-3 fault kills only the faulting task. But all tasks still share one
  page directory, so user tasks can read each other's stacks and the shared
  user code region. Per-process address spaces come with ELF loading.
- **No demand paging, swapping, or copy-on-write.** Every mapped page
  is backed by a real frame up front; a page fault in the kernel is always
  a bug and always panics.
- **Filesystem supports persistent read/write** but its 4 MiB size and
  64-entry limit are fixed at build time.
- **No ELF loading.** User-mode code lives in `kernel/user/`, is linked
  into the kernel image's user region and jumped to directly; there's no
  loader for separately-compiled user binaries.
- **Single CPU, no SMP.**
