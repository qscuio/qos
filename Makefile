ARCHES := x86_64 aarch64
ARCH_GOAL := $(firstword $(filter $(ARCHES),$(MAKECMDGOALS)))

.PHONY: c rust test-all clean $(ARCHES)

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

clean:
	@$(MAKE) -C c-os clean
	@$(MAKE) -C rust-os clean

$(ARCHES):
	@:
