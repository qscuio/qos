// SPDX-License-Identifier: GPL-2.0

//! Rust slab sample.
//!
//! Intended interface:
//! - `/dev/rust-slab`
//! - `ALLOC = _IOR('Q', 0xC0, u64)`
//! - `FREE = _IOW('Q', 0xC1, u64)`
//! - `FILL = _IOW('Q', 0xC2, struct FillRequest)`
//! - `STATS = _IOR('Q', 0xC3, struct SlabStats)`
//!
//! The core of the sample is the handle table and generation counter logic
//! that keeps userspace away from raw pointers.

use kernel::prelude::*;

const SLOT_COUNT: usize = 32;
const PAYLOAD_LEN: usize = 56;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
struct Packet {
    header: u32,
    len: u16,
    checksum: u16,
    payload: [u8; PAYLOAD_LEN],
}

impl Packet {
    const fn zeroed() -> Self {
        Self {
            header: 0,
            len: 0,
            checksum: 0,
            payload: [0; PAYLOAD_LEN],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct SlabStats {
    active: u32,
    total: u32,
    highwater: u32,
}

#[derive(Clone, Copy, Debug)]
struct Slot {
    generation: u32,
    live: bool,
    packet: Packet,
}

impl Slot {
    const fn empty() -> Self {
        Self {
            generation: 0,
            live: false,
            packet: Packet::zeroed(),
        }
    }
}

struct SlabState {
    slots: [Slot; SLOT_COUNT],
    stats: SlabStats,
}

impl SlabState {
    const fn new() -> Self {
        Self {
            slots: [Slot::empty(); SLOT_COUNT],
            stats: SlabStats {
                active: 0,
                total: 0,
                highwater: 0,
            },
        }
    }

    fn alloc(&mut self) -> Result<u64> {
        let (index, slot) = self
            .slots
            .iter_mut()
            .enumerate()
            .find(|(_, slot)| !slot.live)
            .ok_or(ENOSPC)?;
        slot.live = true;
        slot.generation = slot.generation.wrapping_add(1);
        slot.packet = Packet::zeroed();
        self.stats.active += 1;
        self.stats.total += 1;
        self.stats.highwater = self.stats.highwater.max(self.stats.active);
        Ok(((slot.generation as u64) << 32) | index as u64)
    }

    fn free(&mut self, handle: u64) -> Result {
        let index = (handle & 0xffff_ffff) as usize;
        let generation = (handle >> 32) as u32;
        let slot = self.slots.get_mut(index).ok_or(EINVAL)?;
        if !slot.live || slot.generation != generation {
            return Err(ENOENT);
        }
        slot.live = false;
        slot.packet = Packet::zeroed();
        self.stats.active -= 1;
        Ok(())
    }

    fn fill(&mut self, handle: u64, byte: u8) -> Result {
        let index = (handle & 0xffff_ffff) as usize;
        let generation = (handle >> 32) as u32;
        let slot = self.slots.get_mut(index).ok_or(EINVAL)?;
        if !slot.live || slot.generation != generation {
            return Err(ENOENT);
        }
        slot.packet.payload.fill(byte);
        slot.packet.len = PAYLOAD_LEN as u16;
        slot.packet.header = 0x504b_5400 | byte as u32;
        Ok(())
    }
}

struct SlabBoundary;

impl SlabBoundary {
    unsafe fn describe_cache_ops() {
        // SAFETY: This sample boundary does not dereference foreign pointers; it
        // only marks where kmem_cache_* wrappers are expected to live.
        pr_info!("rust_slab: kmem_cache_* FFI boundary is isolated here\n");
    }
}

module! {
    type: RustSlab,
    name: "rust_slab",
    authors: ["Qulk Learning"],
    description: "Rust slab learning module",
    license: "GPL",
}

struct RustSlab {
    state: SlabState,
}

impl kernel::Module for RustSlab {
    fn init(_module: &'static ThisModule) -> Result<Self> {
        let mut state = SlabState::new();
        let handle = state.alloc()?;
        state.fill(handle, 0x41)?;

        unsafe {
            SlabBoundary::describe_cache_ops();
        }

        pr_info!("rust_slab: bootstrap handle allocated {:x}\n", handle);
        Ok(Self { state })
    }
}

impl Drop for RustSlab {
    fn drop(&mut self) {
        for index in 0..SLOT_COUNT {
            if self.state.slots[index].live {
                let handle =
                    ((self.state.slots[index].generation as u64) << 32) | index as u64;
                let _ = self.state.free(handle);
                pr_warn!("rust_slab: freeing live packet handle {:x} during unload\n", handle);
            }
        }
        pr_info!("rust_slab: final stats {:?}\n", self.state.stats);
    }
}
