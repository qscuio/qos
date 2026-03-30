// SPDX-License-Identifier: GPL-2.0

//! Rust platform-driver sample.
//!
//! Intended interface:
//! - platform device name: `rust-platform-demo`
//! - misc device: `/dev/rust-platform`
//! - `REG_READ = _IOR('Q', 0xB0, u32)`
//! - `REG_WRITE = _IOW('Q', 0xB1, struct PlatformWrite)`
//!
//! This file keeps the simulated register model and lifecycle notes together so
//! the platform glue can wrap a small, focused core.

use alloc::format;
use kernel::prelude::*;

const REGISTER_COUNT: usize = 16;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct PlatformWrite {
    index: u32,
    value: u32,
}

#[derive(Debug)]
struct PlatformRegisters {
    regs: [u32; REGISTER_COUNT],
    probe_count: u64,
    remove_count: u64,
}

impl PlatformRegisters {
    const fn new() -> Self {
        Self {
            regs: [0; REGISTER_COUNT],
            probe_count: 0,
            remove_count: 0,
        }
    }

    fn probe(&mut self) {
        self.probe_count += 1;
    }

    fn remove(&mut self) {
        self.remove_count += 1;
    }

    fn write_reg(&mut self, index: usize, value: u32) -> Result {
        let slot = self.regs.get_mut(index).ok_or(EINVAL)?;
        *slot = value;
        Ok(())
    }

    fn read_reg(&self, index: usize) -> Result<u32> {
        self.regs.get(index).copied().ok_or(EINVAL)
    }

    fn summary(&self) -> alloc::string::String {
        format!(
            "probes={} removes={} first_reg={:#x}",
            self.probe_count, self.remove_count, self.regs[0]
        )
    }
}

struct PlatformBoundary;

impl PlatformBoundary {
    unsafe fn describe_probe_remove() {
        // SAFETY: The placeholder boundary does not touch foreign memory. It
        // exists only to keep the future FFI and safety argument localized.
        pr_info!("rust_platform: platform::Driver bridge belongs here\n");
    }
}

module! {
    type: RustPlatform,
    name: "rust_platform",
    authors: ["Qulk Learning"],
    description: "Rust platform driver learning module",
    license: "GPL",
}

struct RustPlatform {
    regs: PlatformRegisters,
}

impl kernel::Module for RustPlatform {
    fn init(_module: &'static ThisModule) -> Result<Self> {
        let mut regs = PlatformRegisters::new();
        regs.probe();
        regs.write_reg(0, 0xfeed_cafe)?;
        regs.write_reg(1, 0x0000_0010)?;

        unsafe {
            PlatformBoundary::describe_probe_remove();
        }

        pr_info!("rust_platform: {}\n", regs.summary());
        pr_info!("rust_platform: intended device is /dev/rust-platform\n");
        Ok(Self { regs })
    }
}

impl Drop for RustPlatform {
    fn drop(&mut self) {
        self.regs.remove();
        pr_info!(
            "rust_platform: removed simulated device (reg0={:#x})\n",
            self.regs.read_reg(0).unwrap_or_default()
        );
    }
}
