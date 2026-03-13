ARCHES := x86_64 aarch64
ARCH_GOAL := $(firstword $(filter $(ARCHES),$(MAKECMDGOALS)))

.PHONY: c rust test-all xv6 xv6-clean test-xv6 clean $(ARCHES)

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

clean:
	@$(MAKE) -C c-os clean
	@$(MAKE) -C rust-os clean
	@$(MAKE) -C xv6 TOOLPREFIX=i686-linux-gnu- clean

$(ARCHES):
	@:
