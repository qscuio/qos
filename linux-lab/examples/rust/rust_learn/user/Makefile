# SPDX-License-Identifier: GPL-2.0
#
# Build Rust userspace test programs for rust_learn kernel modules.
#
# These can be compiled on the host (same arch as guest) or inside the guest.
#
# Usage:
#   make                  # build all
#   make test_chardev     # build single target
#   make clean

RUSTC ?= rustc-1.80
RUSTFLAGS ?= --edition 2021

TARGETS := test_chardev test_hello test_sync test_workqueue test_platform test_slab

all: $(TARGETS)

test_chardev: test_chardev.rs
	$(RUSTC) $(RUSTFLAGS) -o $@ $<

test_hello: test_hello.rs
	$(RUSTC) $(RUSTFLAGS) -o $@ $<

test_sync: test_sync.rs
	$(RUSTC) $(RUSTFLAGS) -o $@ $<

test_workqueue: test_workqueue.rs
	$(RUSTC) $(RUSTFLAGS) -o $@ $<

test_platform: test_platform.rs
	$(RUSTC) $(RUSTFLAGS) -o $@ $<

test_slab: test_slab.rs
	$(RUSTC) $(RUSTFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
