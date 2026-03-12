ARCH ?= x86_64
SUPPORTED_ARCHES := x86_64 aarch64

ifeq ($(filter $(ARCH),$(SUPPORTED_ARCHES)),)
$(error Unsupported ARCH '$(ARCH)'. Supported: $(SUPPORTED_ARCHES))
endif

BUILD_DIR := build/$(ARCH)
LOG := $(BUILD_DIR)/smoke.log
X86_DISK := $(BUILD_DIR)/disk.img
AARCH64_KERNEL := $(BUILD_DIR)/kernel.elf
AARCH64_INITRD := $(BUILD_DIR)/initramfs.cpio

ifeq ($(ARCH),x86_64)
ARTIFACT := $(X86_DISK)
INITRD :=
else
ARTIFACT := $(AARCH64_KERNEL)
INITRD := $(AARCH64_INITRD)
endif

.PHONY: build smoke clean FORCE

build: $(ARTIFACT) $(INITRD)

$(X86_DISK): FORCE
	@mkdir -p $(BUILD_DIR)
	@../scripts/build_x86_boot_img.sh rust $@

$(AARCH64_KERNEL): FORCE
	@mkdir -p $(BUILD_DIR)
	@../scripts/build_aarch64_kernel.sh rust $@

$(AARCH64_INITRD): FORCE
	@mkdir -p $(BUILD_DIR)
	@../scripts/build_aarch64_initramfs.sh rust $@

smoke: build
ifeq ($(ARCH),aarch64)
	@../scripts/smoke_target.sh rust $(ARCH) $(ARTIFACT) $(LOG) $(INITRD)
else
	@../scripts/smoke_target.sh rust $(ARCH) $(ARTIFACT) $(LOG)
endif

clean:
	@rm -rf build

FORCE:
	@:
