ASM      = nasm
CC       = gcc
LD       = ld
OBJCOPY  = objcopy
QEMU     = qemu-system-i386

BUILD_DIR = build

CFLAGS  = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -fno-builtin \
          -Wall -Wextra -Ikernel/include -c
LDFLAGS = -m elf_i386 -T kernel/linker.ld -nostdlib

KERNEL_C_SRCS  = $(wildcard kernel/*.c) $(wildcard kernel/bin/*.c)
KERNEL_ASM_SRCS = $(wildcard kernel/*.asm)
KERNEL_OBJS = $(patsubst kernel/%.c,$(BUILD_DIR)/%.o,$(KERNEL_C_SRCS)) \
              $(patsubst kernel/%.asm,$(BUILD_DIR)/%.o,$(KERNEL_ASM_SRCS))
KERNEL_HEADERS = $(wildcard kernel/include/*.h)

FS_FILES = $(wildcard fs/*)

.PHONY: all clean run run-headless

all: $(BUILD_DIR)/TheOS.img

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.bin: boot/boot.asm | $(BUILD_DIR)
	$(ASM) -f bin $< -o $@

$(BUILD_DIR)/stage2.bin: boot/stage2.asm | $(BUILD_DIR)
	$(ASM) -f bin $< -o $@

$(BUILD_DIR)/%.o: kernel/%.c $(KERNEL_HEADERS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: kernel/%.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS) kernel/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/fs.img: scripts/build_fs.sh $(FS_FILES) | $(BUILD_DIR)
	bash scripts/build_fs.sh $(BUILD_DIR) fs

$(BUILD_DIR)/TheOS.img: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/stage2.bin $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/fs.img scripts/build_image.sh
	bash scripts/build_image.sh $(BUILD_DIR)

run: all
	$(QEMU) -drive format=raw,file=$(BUILD_DIR)/TheOS.img

run-headless: all
	$(QEMU) -drive format=raw,file=$(BUILD_DIR)/TheOS.img -display none -serial stdio -no-reboot

clean:
	rm -rf $(BUILD_DIR)
