# Disk layout. KERNEL_SECTORS and FS_BASE_LBA must match stage2.asm and fs.h.
SECTOR_SIZE=512
STAGE2_SECTORS=4
KERNEL_SECTORS=256
FS_DIR_SECTORS=6          # 1 superblock + 5 directory sectors
FS_TOTAL_SECTORS=8192     # 4 MiB reserved for the whole TOFS region (data + free space)

BOOT_LBA=0
STAGE2_LBA=1
KERNEL_LBA=$(( STAGE2_LBA + STAGE2_SECTORS ))
FS_BASE_LBA=$(( KERNEL_LBA + KERNEL_SECTORS ))
