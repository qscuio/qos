// SPDX-License-Identifier: GPL-2.0

//! Rust sync sample.
//!
//! Intended interface:
//! - `/dev/rust-sync`
//! - `SYNC_PUSH = _IOW('Q', 0x90, u64)`
//! - `SYNC_POP = _IOR('Q', 0x91, u64)`
//! - `SYNC_STATS = _IOR('Q', 0x92, struct SyncStats)`
//!
//! This sample keeps the data-structure and lifecycle logic close to the
//! surface described by the design spec. The misc-device wiring is meant to
//! sit on top of these helpers when built in the target kernel tree.

use kernel::prelude::*;

const RING_CAPACITY: usize = 64;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct SyncStats {
    push_count: u64,
    pop_count: u64,
    high_watermark: u32,
    depth: u32,
}

#[derive(Debug)]
struct RingBuffer {
    values: [u64; RING_CAPACITY],
    head: usize,
    len: usize,
}

impl RingBuffer {
    const fn new() -> Self {
        Self {
            values: [0; RING_CAPACITY],
            head: 0,
            len: 0,
        }
    }

    fn depth(&self) -> usize {
        self.len
    }

    fn push(&mut self, value: u64) -> Result {
        if self.len == RING_CAPACITY {
            return Err(EBUSY);
        }
        let tail = (self.head + self.len) % RING_CAPACITY;
        self.values[tail] = value;
        self.len += 1;
        Ok(())
    }

    fn pop(&mut self) -> Result<u64> {
        if self.len == 0 {
            return Err(EAGAIN);
        }
        let value = self.values[self.head];
        self.head = (self.head + 1) % RING_CAPACITY;
        self.len -= 1;
        Ok(value)
    }
}

#[derive(Debug)]
struct SyncState {
    ring: RingBuffer,
    stats: SyncStats,
}

impl SyncState {
    const fn new() -> Self {
        Self {
            ring: RingBuffer::new(),
            stats: SyncStats {
                push_count: 0,
                pop_count: 0,
                high_watermark: 0,
                depth: 0,
            },
        }
    }

    fn push(&mut self, value: u64) -> Result {
        self.ring.push(value)?;
        self.stats.push_count += 1;
        self.stats.depth = self.ring.depth() as u32;
        self.stats.high_watermark = self.stats.high_watermark.max(self.stats.depth);
        Ok(())
    }

    fn pop(&mut self) -> Result<u64> {
        let value = self.ring.pop()?;
        self.stats.pop_count += 1;
        self.stats.depth = self.ring.depth() as u32;
        Ok(value)
    }

    fn stats(&self) -> SyncStats {
        self.stats
    }
}

module! {
    type: RustSync,
    name: "rust_sync",
    authors: ["Qulk Learning"],
    description: "Rust sync learning module",
    license: "GPL",
}

struct RustSync {
    state: SyncState,
}

impl kernel::Module for RustSync {
    fn init(_module: &'static ThisModule) -> Result<Self> {
        let mut state = SyncState::new();
        state.push(1)?;
        state.push(2)?;
        let _ = state.pop()?;

        pr_info!("rust_sync: initialized shared ring-buffer teaching state\n");
        pr_info!("rust_sync: target device path is /dev/rust-sync\n");
        Ok(Self { state })
    }
}

impl Drop for RustSync {
    fn drop(&mut self) {
        while let Ok(value) = self.state.pop() {
            pr_info!("rust_sync: draining queued value {}\n", value);
        }
        pr_info!("rust_sync: final stats {:?}\n", self.state.stats());
    }
}
