ARCHES := x86_64 aarch64
ARCH_GOAL := $(firstword $(filter $(ARCHES),$(MAKECMDGOALS)))
LINUX_LAB_KERNEL ?= 6.18.4
LINUX_LAB_ARCH ?= x86_64
LINUX_LAB_IMAGE ?= noble
LINUX_LAB_PROFILES ?= default-lab
LINUX_LAB_MIRROR ?=
LINUX_LAB_PROFILE_ARGS := $(foreach profile,$(LINUX_LAB_PROFILES),--profile $(profile))
LINUX_LAB_MIRROR_ARG := $(if $(strip $(LINUX_LAB_MIRROR)),--mirror $(LINUX_LAB_MIRROR),)

.PHONY: c rust test-all xv6 xv6-clean test-xv6 linux1 linux1-curses test-linux1 linux1-clean linux-lab-validate linux-lab-plan linux-lab-run clean $(ARCHES)

c:
	@if [ -z "$(ARCH_GOAL)" ]; then \
		echo "usage: make c <x86_64|aarch64>"; \
		exit 2; \
	fi
	@$(MAKE) -C c-os ARCH=$(ARCH_GOAL) build

rust:
	@if [ -z "$(ARCH_GOAL)" ]; then \
		echo "usage: make rust <x86_64|aarch64>"; \
		exit 2; \
	fi
	@$(MAKE) -C rust-os ARCH=$(ARCH_GOAL) build

test-all:
	@$(MAKE) -C c-os ARCH=x86_64 smoke
	@$(MAKE) -C c-os ARCH=aarch64 smoke
	@$(MAKE) -C rust-os ARCH=x86_64 smoke
	@$(MAKE) -C rust-os ARCH=aarch64 smoke
	@bash scripts/xv6/build.sh
	@pytest tests/test_xv6.py -v

xv6:
	@bash scripts/xv6/build.sh

xv6-clean:
	@$(MAKE) -C xv6 TOOLPREFIX=i686-linux-gnu- clean

test-xv6:
	@pytest tests/test_xv6.py -v

linux1:
	@bash scripts/linux1/fetch_sources.sh
	@bash scripts/linux1/build_kernel.sh
	@bash scripts/linux1/build_userspace.sh
	@bash scripts/linux1/build_lilo.sh
	@bash scripts/linux1/mk_disk.sh
	@bash scripts/linux1/run_qemu.sh

linux1-curses:
	@bash scripts/linux1/fetch_sources.sh
	@bash scripts/linux1/build_kernel.sh
	@bash scripts/linux1/build_userspace.sh
	@bash scripts/linux1/build_lilo.sh
	@bash scripts/linux1/mk_disk.sh
	@bash scripts/linux1/run_curses.sh

test-linux1:
	@pytest tests/test_linux1.py -v

linux-lab-validate:
	@linux-lab/bin/linux-lab validate --kernel $(LINUX_LAB_KERNEL) --arch $(LINUX_LAB_ARCH) --image $(LINUX_LAB_IMAGE) $(LINUX_LAB_PROFILE_ARGS) $(LINUX_LAB_MIRROR_ARG)

linux-lab-plan:
	@linux-lab/bin/linux-lab plan --kernel $(LINUX_LAB_KERNEL) --arch $(LINUX_LAB_ARCH) --image $(LINUX_LAB_IMAGE) $(LINUX_LAB_PROFILE_ARGS) $(LINUX_LAB_MIRROR_ARG)

linux-lab-run:
	@linux-lab/bin/linux-lab run --kernel $(LINUX_LAB_KERNEL) --arch $(LINUX_LAB_ARCH) --image $(LINUX_LAB_IMAGE) $(LINUX_LAB_PROFILE_ARGS) $(LINUX_LAB_MIRROR_ARG) --dry-run

linux1-clean:
	@rm -rf build/linux1

clean:
	@$(MAKE) -C c-os clean
	@$(MAKE) -C rust-os clean
	@$(MAKE) -C xv6 TOOLPREFIX=i686-linux-gnu- clean

$(ARCHES):
	@:
