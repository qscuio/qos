// SPDX-License-Identifier: GPL-2.0

//! Hello Rust - A learning kernel module in Rust.
//!
//! This module demonstrates the basics of writing a Rust kernel module:
//! - The `module!` macro for module metadata
//! - Module parameters
//! - `pr_info!` for kernel logging
//! - The `Module` trait for initialization
//! - `Drop` for cleanup on module unload
//!
//! Usage:
//!   insmod hello_rust.ko
//!   insmod hello_rust.ko greeting_count=5
//!   dmesg | grep hello_rust
//!   rmmod hello_rust

use kernel::prelude::*;

module! {
    type: HelloRust,
    name: "hello_rust",
    authors: ["Qulk Learning"],
    description: "A simple hello world Rust kernel module",
    license: "GPL",
    params: {
        greeting_count: i32 {
            default: 3,
            description: "Number of greetings to print",
        },
    },
}

struct HelloRust {
    count: i32,
}

impl kernel::Module for HelloRust {
    fn init(_module: &'static ThisModule) -> Result<Self> {
        let count = *module_parameters::greeting_count.value();
        pr_info!("Hello from Rust! (greeting_count={})\n", count);
        pr_info!("Am I built-in? {}\n", !cfg!(MODULE));

        for i in 0..count {
            pr_info!("  Greeting #{}\n", i + 1);
        }

        // Demonstrate KVec (kernel vector) usage
        let mut numbers = KVec::new();
        numbers.push(42, GFP_KERNEL)?;
        numbers.push(99, GFP_KERNEL)?;
        pr_info!("Created a KVec with {} elements: {:?}\n", numbers.len(), numbers);

        Ok(HelloRust { count })
    }
}

impl Drop for HelloRust {
    fn drop(&mut self) {
        pr_info!("Goodbye from Rust! (printed {} greetings)\n", self.count);
    }
}
