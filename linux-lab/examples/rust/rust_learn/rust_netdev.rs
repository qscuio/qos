// SPDX-License-Identifier: GPL-2.0

//! Rust netdev sample.
//!
//! Intended interface:
//! - network interface: `rustnet0`
//! - echo mode loops frames back through the RX path
//! - carrier state models `netif_carrier_on/off`
//!
//! The sample keeps the packet-accounting and carrier-state logic in plain
//! Rust structures so the eventual C-facing registration layer has a narrow
//! surface area.

use kernel::prelude::*;

#[derive(Clone, Copy, Debug, Default)]
struct NetdevStats {
    tx_packets: u64,
    rx_packets: u64,
    tx_bytes: u64,
    rx_bytes: u64,
    drops: u64,
}

#[derive(Clone, Copy, Debug)]
enum EchoMode {
    Echo,
    Drop,
}

struct NetdevState {
    carrier_up: bool,
    mode: EchoMode,
    stats: NetdevStats,
}

impl NetdevState {
    const fn new() -> Self {
        Self {
            carrier_up: false,
            mode: EchoMode::Echo,
            stats: NetdevStats {
                tx_packets: 0,
                rx_packets: 0,
                tx_bytes: 0,
                rx_bytes: 0,
                drops: 0,
            },
        }
    }

    fn set_carrier(&mut self, up: bool) {
        self.carrier_up = up;
    }

    fn set_mode(&mut self, mode: EchoMode) {
        self.mode = mode;
    }

    fn transmit(&mut self, bytes: usize) {
        self.stats.tx_packets += 1;
        self.stats.tx_bytes += bytes as u64;
        match self.mode {
            EchoMode::Echo if self.carrier_up => {
                self.stats.rx_packets += 1;
                self.stats.rx_bytes += bytes as u64;
            }
            _ => {
                self.stats.drops += 1;
            }
        }
    }
}

struct NetdevBoundary;

impl NetdevBoundary {
    unsafe fn describe_registration() {
        // SAFETY: This placeholder function does not touch foreign pointers. It
        // exists only to keep the alloc_netdev/register_netdev bridge localized.
        pr_info!("rust_netdev: alloc_netdev/register_netdev bridge belongs here\n");
    }
}

module! {
    type: RustNetdev,
    name: "rust_netdev",
    authors: ["Qulk Learning"],
    description: "Rust netdev learning module",
    license: "GPL",
}

struct RustNetdev {
    state: NetdevState,
}

impl kernel::Module for RustNetdev {
    fn init(_module: &'static ThisModule) -> Result<Self> {
        let mut state = NetdevState::new();
        state.set_carrier(true);
        state.set_mode(EchoMode::Echo);
        state.transmit(64);
        state.set_mode(EchoMode::Drop);
        state.transmit(128);

        unsafe {
            NetdevBoundary::describe_registration();
        }

        pr_info!("rust_netdev: interface rustnet0 teaching state initialized\n");
        pr_info!("rust_netdev: stats {:?}\n", state.stats);
        Ok(Self { state })
    }
}

impl Drop for RustNetdev {
    fn drop(&mut self) {
        pr_info!("rust_netdev: final stats {:?}\n", self.state.stats);
    }
}
