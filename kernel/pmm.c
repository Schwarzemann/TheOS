#include <pmm.h>
#include <string.h>
#include <panic.h>

#define PMM_MAX_FRAMES (256 * 1024)

extern uint32_t kernel_end;

static uint8_t bitmap[PMM_MAX_FRAMES / 8];
static uint32_t total_frames;
static uint32_t free_count;

static void set_used(uint32_t frame) {
    bitmap[frame / 8] |= (uint8_t)(1 << (frame % 8));
}

static void set_free(uint32_t frame) {
    bitmap[frame / 8] &= (uint8_t)~(1 << (frame % 8));
}

static int is_used(uint32_t frame) {
    return (bitmap[frame / 8] >> (frame % 8)) & 1;
}

void pmm_init(uint32_t mem_kb) {
    uint32_t frames = (mem_kb * 1024) / PMM_FRAME_SIZE;
    if (frames > PMM_MAX_FRAMES) {
        frames = PMM_MAX_FRAMES;
    }
    total_frames = frames;

    memset(bitmap, 0xFF, sizeof(bitmap)); /* everything reserved by default */
    free_count = 0;

    uint32_t end_frame = ((uint32_t)&kernel_end + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;
    for (uint32_t frame = end_frame; frame < total_frames; frame++) {
        set_free(frame);
        free_count++;
    }
}

uint32_t pmm_alloc(void) {
    for (uint32_t frame = 0; frame < total_frames; frame++) {
        if (!is_used(frame)) {
            set_used(frame);
            free_count--;
            return frame * PMM_FRAME_SIZE;
        }
    }
    return 0;
}

void pmm_free(uint32_t phys_addr) {
    uint32_t frame = phys_addr / PMM_FRAME_SIZE;
    if (frame >= total_frames) {
        panic("pmm_free: frame out of range");
    }
    if (!is_used(frame)) {
        panic("pmm_free: double free");
    }
    set_free(frame);
    free_count++;
}

uint32_t pmm_total(void) {
    return total_frames;
}

uint32_t pmm_free_count(void) {
    return free_count;
}
