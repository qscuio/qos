// SPDX-License-Identifier: GPL-2.0

//! Rust workqueue sample.
//!
//! Intended interface:
//! - `/dev/rust-workqueue`
//! - `write(2)` queues a small message for deferred execution
//! - `GET_COMPLETED = _IOR('Q', 0xA0, u64)` returns the completion count
//!
//! The implementation here focuses on the state transitions and accounting the
//! companion userspace helper expects to exercise.

use alloc::vec::Vec;
use kernel::prelude::*;

const MAX_MESSAGE_LEN: usize = 48;

#[derive(Clone, Debug)]
struct WorkRecord {
    id: u64,
    len: usize,
    bytes: [u8; MAX_MESSAGE_LEN],
}

impl WorkRecord {
    fn new(id: u64, message: &[u8]) -> Self {
        let mut bytes = [0_u8; MAX_MESSAGE_LEN];
        let len = core::cmp::min(message.len(), MAX_MESSAGE_LEN);
        bytes[..len].copy_from_slice(&message[..len]);
        Self { id, len, bytes }
    }

    fn message(&self) -> &[u8] {
        &self.bytes[..self.len]
    }
}

#[derive(Default)]
struct WorkqueueState {
    submitted: u64,
    completed: u64,
    pending: Vec<WorkRecord>,
}

impl WorkqueueState {
    fn queue_message(&mut self, message: &[u8]) -> Result {
        self.submitted += 1;
        self.pending.try_push(WorkRecord::new(self.submitted, message))?;
        Ok(())
    }

    fn flush(&mut self) {
        for item in self.pending.drain(..) {
            self.completed += 1;
            pr_info!(
                "rust_workqueue: completed item {} with payload {:?}\n",
                item.id,
                item.message()
            );
        }
    }
}

module! {
    type: RustWorkqueue,
    name: "rust_workqueue",
    authors: ["Qulk Learning"],
    description: "Rust workqueue learning module",
    license: "GPL",
}

struct RustWorkqueue {
    state: WorkqueueState,
}

impl kernel::Module for RustWorkqueue {
    fn init(_module: &'static ThisModule) -> Result<Self> {
        let mut state = WorkqueueState::default();
        state.queue_message(b"bootstrap work item")?;
        state.flush();
        pr_info!("rust_workqueue: initialized deferred-work teaching state\n");
        Ok(Self { state })
    }
}

impl Drop for RustWorkqueue {
    fn drop(&mut self) {
        self.state.flush();
        pr_info!(
            "rust_workqueue: flushed pending work on unload (completed={})\n",
            self.state.completed
        );
    }
}
