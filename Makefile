ARCHES := x86_64 aarch64
ARCH_GOAL := $(firstword $(filter $(ARCHES),$(MAKECMDGOALS)))

.PHONY: c rust test-all xv6 xv6-clean test-xv6 linux1 linux1-curses test-linux1 linux1-clean clean $(ARCHES)

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
	@bash scripts/build_xv6.sh
	@pytest tests/test_xv6.py -v

xv6:
	@bash scripts/build_xv6.sh

xv6-clean:
	@$(MAKE) -C xv6 TOOLPREFIX=i686-linux-gnu- clean

test-xv6:
	@pytest tests/test_xv6.py -v

linux1:
	@bash scripts/fetch_linux1_sources.sh
	@bash scripts/build_linux1_kernel.sh
	@bash scripts/build_linux1_userspace.sh
	@bash scripts/build_linux1_lilo.sh
	@bash scripts/mk_linux1_disk.sh
	@bash scripts/run_linux1_qemu.sh

linux1-curses:
	@bash scripts/fetch_linux1_sources.sh
	@bash scripts/build_linux1_kernel.sh
	@bash scripts/build_linux1_userspace.sh
	@bash scripts/build_linux1_lilo.sh
	@bash scripts/mk_linux1_disk.sh
	@bash scripts/run_linux1_curses.sh

test-linux1:
	@pytest tests/test_linux1.py -v

linux1-clean:
	@rm -rf build/linux1

clean:
	@$(MAKE) -C c-os clean
	@$(MAKE) -C rust-os clean
	@$(MAKE) -C xv6 TOOLPREFIX=i686-linux-gnu- clean

$(ARCHES):
	@:
