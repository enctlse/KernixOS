include common.mk

SRCS = $(shell find $(SRC_DIR) shared -name "*.c" -or -name "*.cpp" -or -name "*.asm")
OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)

.PHONY: all run clean assets
all: limine/limine assets $(ISO)

assets:




$(BUILD_DIR)/kernel.elf: core/kernel/linker.ld $(OBJS)
	$(VLD) $(LDFLAGS) -T $< $(OBJS) -o $@

$(ISO): limine.conf $(BUILD_DIR)/kernel.elf
	@echo "[ISO] Creating bootable image..."
	@rm -rf $(ISODIR)
	@mkdir -p $(ISODIR)/boot/limine $(ISODIR)/EFI/BOOT
	@cp $(BUILD_DIR)/kernel.elf $(ISODIR)/boot/
	@cp $< $(ISODIR)/boot/limine/
	@cp $(addprefix limine/limine-, bios.sys bios-cd.bin uefi-cd.bin) $(ISODIR)/boot/limine/
	@cp $(addprefix limine/BOOT, IA32.EFI X64.EFI) $(ISODIR)/EFI/BOOT/

	@mkdir -p $(ISODIR)/boot
	@mkdir -p $(ISODIR)/boot/ui
	@mkdir -p $(ISODIR)/boot/ui/fonts
	@mkdir -p $(ISODIR)/boot/ui/assets
	@mkdir -p $(ISODIR)/boot/ui/assets/cursor
	@mkdir -p $(ISODIR)/boot/modules

	
	@cp shared/ui/assets/cursor/cursor_small.bmp $(ISODIR)/boot/ui/assets/cursor/ 2>/dev/null || true
	@cp core/gui/wallpaper.bmp $(ISODIR)/boot/ui/assets/ 2>/dev/null || true

	@xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISODIR) -o $@ 2>/dev/null
	@echo "------------------------"
	@echo "[OK]  $@ created"

run: $(ISO)
	@echo "[QEMU]running $(OS_NAME).iso "
	@echo
	@qemu-system-x86_64 \
		-M q35 \
		-cpu qemu64 \
		-m 512 \
		-drive if=pflash,format=raw,readonly=on,file=uefi/OVMF_CODE.fd \
		-drive if=pflash,format=raw,file=uefi/OVMF_VARS.fd \
		-cdrom $< \
		-usb \
		-device usb-tablet \
		-display sdl \
		-serial stdio 2>&1 \
		-no-reboot \
		-no-shutdown \
		-smp 4

limine/limine:
	rm -rf limine
	git clone https://codeberg.org/Limine/Limine.git limine --branch=v10.x-binary --depth=1
	$(MAKE) -C limine \
		CC="$(CC)"
	git clone https://codeberg.org/Limine/limine-protocol.git --depth 1 

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(VCC) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(VCXX) $(CXXFLAGS) -c $< -o $@
$(BUILD_DIR)/%.asm.o: %.asm
	@mkdir -p $(dir $@)
	$(VAS) $(ASFLAGS) $< -o $@

# Clean all build output
clean:
	@echo "[CLR] Cleaning..."
	@rm -rf $(BUILD_DIR)
	@echo "[OK]"
