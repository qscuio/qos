// SPDX-License-Identifier: GPL-2.0

//! Rust procfs sample.
//!
//! Intended interface:
//! - `/proc/rust_procfs/stats`
//! - `/proc/rust_procfs/config`
//!
//! The logic below keeps the seq-style stats and key/value config handling in
//! Rust-owned types. The procfs FFI shim is represented as a tiny wrapper so
//! the safety boundary is visible in one place.

use alloc::{format, string::String, vec::Vec};
use kernel::prelude::*;

const MAX_CONFIG_ENTRIES: usize = 16;

#[derive(Clone, Debug)]
struct ConfigEntry {
    key: String,
    value: String,
}

#[derive(Clone, Copy, Debug, Default)]
struct ProcfsStats {
    read_count: u64,
    open_count: u64,
    uptime_ticks: u64,
}

#[derive(Default)]
struct ProcfsState {
    stats: ProcfsStats,
    config: Vec<ConfigEntry>,
}

impl ProcfsState {
    fn record_open(&mut self) {
        self.stats.open_count += 1;
    }

    fn record_read(&mut self) {
        self.stats.read_count += 1;
        self.stats.uptime_ticks += 1;
    }

    fn apply_line(&mut self, line: &str) -> Result {
        let (key, value) = line.split_once('=').ok_or(EINVAL)?;
        if let Some(entry) = self.config.iter_mut().find(|entry| entry.key == key) {
            entry.value = value.to_owned();
            return Ok(());
        }
        if self.config.len() >= MAX_CONFIG_ENTRIES {
            return Err(ENOSPC);
        }
        self.config.try_push(ConfigEntry {
            key: key.to_owned(),
            value: value.to_owned(),
        })?;
        Ok(())
    }

    fn render_stats(&self) -> String {
        format!(
            "read_count={}\nopen_count={}\nuptime={}\n",
            self.stats.read_count, self.stats.open_count, self.stats.uptime_ticks
        )
    }

    fn render_config(&self) -> String {
        let mut out = String::new();
        for entry in &self.config {
            out.push_str(&entry.key);
            out.push('=');
            out.push_str(&entry.value);
            out.push('\n');
        }
        out
    }
}

struct ProcfsBoundary;

impl ProcfsBoundary {
    unsafe fn describe_registration() {
        // SAFETY: This helper deliberately does not dereference pointers or call
        // into C; it exists so the procfs/seq_file boundary is documented in the
        // same place the real bindings would live when compiled in-tree.
        pr_info!("rust_procfs: proc_create/seq_file boundary is isolated here\n");
    }
}

module! {
    type: RustProcfs,
    name: "rust_procfs",
    authors: ["Qulk Learning"],
    description: "Rust procfs learning module",
    license: "GPL",
}

struct RustProcfs {
    state: ProcfsState,
}

impl kernel::Module for RustProcfs {
    fn init(_module: &'static ThisModule) -> Result<Self> {
        let mut state = ProcfsState::default();
        state.record_open();
        state.record_read();
        state.apply_line("echo_mode=on")?;
        state.apply_line("max_entries=16")?;

        unsafe {
            ProcfsBoundary::describe_registration();
        }

        pr_info!("rust_procfs: stats\n{}\n", state.render_stats());
        pr_info!("rust_procfs: config\n{}\n", state.render_config());
        Ok(Self { state })
    }
}

impl Drop for RustProcfs {
    fn drop(&mut self) {
        pr_info!(
            "rust_procfs: removing proc subtree with {} config entries\n",
            self.state.config.len()
        );
    }
}
