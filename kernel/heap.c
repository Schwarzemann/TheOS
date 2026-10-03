#include <heap.h>
#include <pmm.h>
#include <panic.h>
#include <serial.h>
#include <string.h>
#include <stdint.h>

#define HEAP_MAX_FRAMES (1024)        /* up to 4 MiB */
#define HEAP_MIN_FRAMES (64)          /* 256 KiB floor; heap_init() panics below it */
#define HEAP_ALIGN 8

typedef struct blk_hdr {
    size_t size;                  /* usable bytes after this header */
    int used;
    struct blk_hdr *next;
} blk_hdr_t;

static uint8_t *heap_start;
static size_t heap_size;
static blk_hdr_t *first_block;

static size_t align_up(size_t n) {
    return (n + (HEAP_ALIGN - 1)) & ~(size_t)(HEAP_ALIGN - 1);
}

void heap_init(void) {
    uint32_t want_frames = HEAP_MAX_FRAMES;
    uint32_t avail = pmm_free_count();
    if (avail / 2 < want_frames) {
        want_frames = avail / 2; /* leave half of free RAM for everything else */
    }
    if (want_frames < HEAP_MIN_FRAMES) {
        panic("heap_init: not enough free RAM for a kernel heap");
    }

    /* Frames come back contiguous right after paging_init(), so the heap is one flat arena. */
    uint32_t base = pmm_alloc();
    for (uint32_t i = 1; i < want_frames; i++) {
        uint32_t frame = pmm_alloc();
        if (frame != base + i * PMM_FRAME_SIZE) {
            panic("heap_init: PMM frames were not contiguous");
        }
    }

    heap_start = (uint8_t *)base;
    heap_size = (size_t)want_frames * PMM_FRAME_SIZE;

    first_block = (blk_hdr_t *)heap_start;
    first_block->size = heap_size - sizeof(blk_hdr_t);
    first_block->used = 0;
    first_block->next = 0;

    serial_write("TheOS: kernel heap ");
    serial_uint((unsigned int)(heap_size / 1024));
    serial_write(" KB at phys ");
    serial_hex(base);
    serial_write("\n");
}

void *kmalloc(size_t size) {
    if (size == 0) {
        return 0;
    }
    size = align_up(size);

    for (blk_hdr_t *b = first_block; b != 0; b = b->next) {
        if (b->used || b->size < size) {
            continue;
        }

        size_t remainder = b->size - size;
        /* Split only if the leftover can hold a header plus an aligned payload. */
        if (remainder > sizeof(blk_hdr_t) + HEAP_ALIGN) {
            blk_hdr_t *split = (blk_hdr_t *)((uint8_t *)b + sizeof(blk_hdr_t) + size);
            split->size = remainder - sizeof(blk_hdr_t);
            split->used = 0;
            split->next = b->next;

            b->size = size;
            b->next = split;
        }

        b->used = 1;
        return (void *)((uint8_t *)b + sizeof(blk_hdr_t));
    }

    return 0; /* out of heap memory */
}

void kfree(void *ptr) {
    if (ptr == 0) {
        return;
    }
    blk_hdr_t *b = (blk_hdr_t *)((uint8_t *)ptr - sizeof(blk_hdr_t));
    b->used = 0;

    while (b->next != 0 && !b->next->used) {
        b->size += sizeof(blk_hdr_t) + b->next->size;
        b->next = b->next->next;
    }
}

size_t heap_free_bytes(void) {
    size_t total = 0;
    for (blk_hdr_t *b = first_block; b != 0; b = b->next) {
        if (!b->used) {
            total += b->size;
        }
    }
    return total;
}
