ARCHES := x86_64 aarch64
ARCH_GOAL := $(firstword $(filter $(ARCHES),$(MAKECMDGOALS)))
LINUX_LAB_KERNEL ?= 6.18.4
LINUX_LAB_ARCH ?= x86_64
LINUX_LAB_IMAGE ?= noble
LINUX_LAB_PROFILES ?= default-lab
LINUX_LAB_MIRROR ?=
LINUX_LAB_PROFILE_ARGS := $(foreach profile,$(LINUX_LAB_PROFILES),--profile $(profile))
LINUX_LAB_MIRROR_ARG := $(if $(strip $(LINUX_LAB_MIRROR)),--mirror $(LINUX_LAB_MIRROR),)

# --- syzkaller / fuzzing ---
FUZZ_KERNEL   ?= 6.18.4
FUZZ_ARCH     ?= arm64
FUZZ_IMAGE    ?= noble
GO_VERSION    ?= 1.26.0
QOS_ROOT      := $(shell pwd)
SYZ_SRC       := $(QOS_ROOT)/build/syzkaller/src
SYZ_MGR       := $(SYZ_SRC)/bin/syz-manager
SYZ_SSH_KEY   := $(QOS_ROOT)/syzkaller/ssh/id_rsa
GUEST_HOST    := $(or $(LINUX_LAB_GUEST_HOST),10.10.10.1)
GUEST_USER    := $(or $(LINUX_LAB_GUEST_USER),qwert)
GO_ARCH       := $(shell dpkg --print-architecture 2>/dev/null | sed 's/amd64/amd64/;s/arm64/arm64/' || uname -m | sed 's/x86_64/amd64/;s/aarch64/arm64/')
GO_TARBALL    := go$(GO_VERSION).linux-$(GO_ARCH).tar.gz
GO_URL        := https://dl.google.com/go/$(GO_TARBALL)

.PHONY: c rust test-all xv6 xv6-clean test-xv6 linux1 linux1-curses test-linux1 linux1-clean linux-lab-validate linux-lab-plan linux-lab-run clean $(ARCHES) \
        fuzz fuzz-run fuzz-build fuzz-clean _fuzz-go _fuzz-syzkaller _fuzz-kernel _fuzz-boot _fuzz-ssh-key _fuzz-wait-vm _fuzz-install-key

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

# ---------------------------------------------------------------------------
# Syzkaller fuzzing
#
# Usage:
#   make fuzz-run                   # fully automatic: build + boot VM + fuzz
#   make fuzz                       # build + fuzz (VM must already be running)
#   make fuzz-build                 # build syzkaller + kernel only (no VM)
#   make fuzz-clean                 # wipe corpus/workdir (keeps binaries)
#
# Override defaults:
#   make fuzz-run FUZZ_KERNEL=6.10 GUEST_HOST=192.168.1.5 GUEST_USER=root
# ---------------------------------------------------------------------------

# Fully automatic: build everything, boot the VM, install SSH key, launch.
fuzz-run: fuzz-build _fuzz-boot _fuzz-ssh-key _fuzz-wait-vm _fuzz-install-key
	@echo "==> Launching syzkaller (web UI at http://localhost:56741)..."
	@$(QOS_ROOT)/scripts/start-syzkaller.sh

# Build + fuzz (assumes VM is already running).
fuzz: fuzz-build _fuzz-ssh-key _fuzz-wait-vm _fuzz-install-key
	@echo "==> Launching syzkaller (web UI at http://localhost:56741)..."
	@$(QOS_ROOT)/scripts/start-syzkaller.sh

# Build phase only (safe to run before booting the VM).
fuzz-build: _fuzz-go _fuzz-syzkaller _fuzz-kernel

# Ensure Go >= GO_VERSION is installed.
_fuzz-go:
	@if ! /usr/local/go/bin/go version 2>/dev/null | grep -q "go$(GO_VERSION)"; then \
	    echo "==> Installing Go $(GO_VERSION) for $(GO_ARCH)..."; \
	    cd /tmp && curl -fsSLO "$(GO_URL)" && \
	    sudo tar -C /usr/local -xzf "$(GO_TARBALL)" && \
	    rm -f "$(GO_TARBALL)"; \
	    echo "==> Go $(GO_VERSION) installed."; \
	fi
	@/usr/local/go/bin/go version

# Build syz-manager and the arm64 executor (idempotent).
_fuzz-syzkaller: _fuzz-go
	@if [ ! -x "$(SYZ_MGR)" ]; then \
	    if [ ! -d "$(SYZ_SRC)" ]; then \
	        echo "ERROR: syzkaller source missing at $(SYZ_SRC)" >&2; \
	        echo "  git clone https://github.com/google/syzkaller $(SYZ_SRC)" >&2; \
	        exit 1; \
	    fi; \
	    echo "==> Building syzkaller for linux/$(FUZZ_ARCH)..."; \
	    PATH=/usr/local/go/bin:$$PATH \
	        $(MAKE) -C "$(SYZ_SRC)" TARGETOS=linux TARGETARCH=$(FUZZ_ARCH); \
	fi
	@echo "==> syz-manager ready: $(SYZ_MGR)"

# Build a fuzzing-enabled kernel via the orchestrator (no VM boot).
_fuzz-kernel:
	@echo "==> Building kernel $(FUZZ_KERNEL) $(FUZZ_ARCH) with debug+fuzzing profiles..."
	@cd "$(QOS_ROOT)" && linux-lab/bin/linux-lab run \
	    --kernel $(FUZZ_KERNEL) --arch $(FUZZ_ARCH) --image $(FUZZ_IMAGE) \
	    --profile debug --profile fuzzing --stop-after build-image

# Boot the VM via the orchestrator (full run, includes boot stage).
_fuzz-boot: _fuzz-kernel
	@echo "==> Booting VM with kernel $(FUZZ_KERNEL) $(FUZZ_ARCH)..."
	@cd "$(QOS_ROOT)" && linux-lab/bin/linux-lab run \
	    --kernel $(FUZZ_KERNEL) --arch $(FUZZ_ARCH) --image $(FUZZ_IMAGE) \
	    --profile debug --profile fuzzing

# Generate the SSH keypair in syzkaller/ssh/ (directory is gitignored).
_fuzz-ssh-key:
	@mkdir -p "$(QOS_ROOT)/syzkaller/ssh"
	@if [ ! -f "$(SYZ_SSH_KEY)" ]; then \
	    echo "==> Generating syzkaller SSH key..."; \
	    ssh-keygen -t rsa -b 4096 -f "$(SYZ_SSH_KEY)" -N '' \
	        -C 'syzkaller@linux-lab' -q; \
	    chmod 600 "$(SYZ_SSH_KEY)"; \
	fi

# Wait for the guest's SSH port to become reachable (TCP probe, no auth needed).
_fuzz-wait-vm:
	@echo "==> Waiting for $(GUEST_HOST):22 (up to 180s)..."
	@for i in $$(seq 1 36); do \
	    if timeout 3 bash -c "echo > /dev/tcp/$(GUEST_HOST)/22" 2>/dev/null; then \
	        echo "==> SSH port reachable."; \
	        exit 0; \
	    fi; \
	    printf "   attempt $$i/36...\r"; \
	    sleep 5; \
	done; \
	echo ""; \
	echo "ERROR: Guest $(GUEST_HOST):22 unreachable after 180s." >&2; \
	echo "Boot the VM first, then re-run 'make fuzz'." >&2; \
	exit 1

# Install the syzkaller public key on the guest via linux-lab's own SSH path,
# which already has access to the guest without the syzkaller key.
_fuzz-install-key: _fuzz-wait-vm
	@KEY=$$(cat "$(SYZ_SSH_KEY).pub"); \
	if ! LINUX_LAB_GUEST_HOST=$(GUEST_HOST) LINUX_LAB_GUEST_USER=$(GUEST_USER) \
	    $(QOS_ROOT)/linux-lab/scripts/guest_run \
	        "grep -qF '$$KEY' ~/.ssh/authorized_keys 2>/dev/null" 2>/dev/null; then \
	    echo "==> Installing syzkaller SSH public key on guest..."; \
	    LINUX_LAB_GUEST_HOST=$(GUEST_HOST) LINUX_LAB_GUEST_USER=$(GUEST_USER) \
	    $(QOS_ROOT)/linux-lab/scripts/guest_run \
	        "mkdir -p ~/.ssh && chmod 700 ~/.ssh && \
	         printf '%s\n' '$$KEY' >> ~/.ssh/authorized_keys && \
	         chmod 600 ~/.ssh/authorized_keys"; \
	fi

# Wipe the fuzzing corpus and crash reports; keep the built binaries.
fuzz-clean:
	@rm -rf "$(QOS_ROOT)/build/syzkaller/workdir"
	@echo "==> Syzkaller workdir removed (binaries kept)."
