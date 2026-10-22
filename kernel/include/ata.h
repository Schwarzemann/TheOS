#ifndef THEOS_ATA_H
#define THEOS_ATA_H

#include <stdint.h>

/* PIO driver for the primary bus, master drive. */

/* Reads `count` 512-byte sectors starting at `lba` into buf (count*512 bytes).
 * Panics on an ATA error: there's no recovery path for a failed boot-disk read. */
void ata_read(uint32_t lba, uint32_t count, void *buf);

/* Writes `count` 512-byte sectors starting at `lba` from buf (count*512 bytes),
 * then flushes the drive's write cache. Panics on an ATA error. */
void ata_write(uint32_t lba, uint32_t count, const void *buf);

#endif
