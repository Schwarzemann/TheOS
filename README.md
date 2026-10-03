# TheOS

A small x86 operating system built from scratch.

## Building and running

Requires `nasm`, `gcc` (with 32-bit support, `-m32`), `ld`, `objcopy`,
`python3` and `qemu-system-i386`.

```sh
make all              # build and leave it as it is without running it
make run              # build and boot in QEMU with a graphical window
make run-headless      # build and boot with serial output on stdout, no display
make clean
```

## Known limitations / what's not here

These will be added in the future:

- **No real process isolation.** Ring-3 tasks share the kernel's
  identity-mapped address space (every page is marked user-accessible)
  instead of getting their own page directory. Fine for the current demo,
  not a security boundary.
- **No demand paging, swapping, or copy-on-write.** Every mapped page
  is backed by a real frame up front; a page fault is always a bug and
  always panics.
- **Filesystem is read-only** and fixed at build time - no write
  support, no subdirectories, no dynamic file creation at runtime.
- **No ELF loading.** User-mode code is kernel-linked and jumped to
  directly; there's no loader for separately-compiled user binaries.
- **Single CPU, no SMP.**
