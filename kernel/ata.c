#include <ata.h>
#include <io.h>
#include <panic.h>

#define ATA_DATA        0x1F0
#define ATA_FEATURES    0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

#define ATA_SR_ERR 0x01
#define ATA_SR_DRQ 0x08
#define ATA_SR_BSY 0x80

#define ATA_CMD_READ 0x20

static void ata_select(uint32_t lba) {
    outb(ATA_DRIVE_HEAD, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    /* ATA needs ~400ns after a drive select; each io_wait() is a ~1us port write. */
    for (int i = 0; i < 4; i++) {
        io_wait();
    }
}

static void ata_read_one(uint32_t lba, uint16_t *buf) {
    ata_select(lba);

    outb(ATA_FEATURES, 0x00);
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LOW, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_READ);

    for (;;) {
        uint8_t status = inb(ATA_STATUS);
        if (status & ATA_SR_ERR) {
            panic("ata_read: device reported an error");
        }
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
            break;
        }
    }

    for (int i = 0; i < 256; i++) {
        buf[i] = inw(ATA_DATA);
    }
}

void ata_read(uint32_t lba, uint32_t count, void *buf) {
    uint16_t *dst = (uint16_t *)buf;
    for (uint32_t i = 0; i < count; i++) {
        ata_read_one(lba + i, dst + i * 256);
    }
}
