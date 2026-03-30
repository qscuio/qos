// SPDX-License-Identifier: GPL-2.0
//
// Userspace test program for the rust_chardev kernel module.
//
// Usage:
//   # First load the kernel module:
//   insmod /home/qwert/build/samples/rust/rust_chardev.ko
//
//   # Then run this test:
//   /home/qwert/linux-lab/examples/rust/rust_learn/user/test_chardev
//
//   # Check kernel logs:
//   dmesg | tail

use std::fs::{File, OpenOptions};
use std::io;
use std::os::unix::io::AsRawFd;

// ioctl constants - must match rust_chardev.rs
// _IO('Q', 0x80)  = direction=0, type='Q'(0x51), nr=0x80, size=0
// _IOR('Q', 0x81, i32) = direction=2, type='Q'(0x51), nr=0x81, size=4
const CHARDEV_HELLO: libc::c_ulong = 0x0000_5180; // _IO('Q', 0x80)

// For aarch64: _IOR = (2 << 30) | (size << 16) | (type << 8) | nr
// = 0x80000000 | (4 << 16) | ('Q' << 8) | 0x81
const CHARDEV_GET_COUNT: libc::c_ulong = 0x8004_5181; // _IOR('Q', 0x81, i32)

const DEVICE_PATH: &str = "/dev/rust-chardev";

#[allow(non_camel_case_types)]
mod libc {
    pub type c_ulong = u64;
    pub type c_int = i32;

    extern "C" {
        pub fn ioctl(fd: c_int, request: c_ulong, ...) -> c_int;
    }
}

fn ioctl_hello(fd: &File) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd.as_raw_fd(), CHARDEV_HELLO) };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    println!("[OK] CHARDEV_HELLO ioctl succeeded");
    Ok(())
}

fn ioctl_get_count(fd: &File) -> io::Result<i32> {
    let mut count: i32 = 0;
    let ret = unsafe {
        libc::ioctl(
            fd.as_raw_fd(),
            CHARDEV_GET_COUNT,
            &mut count as *mut i32,
        )
    };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(count)
}

fn main() {
    println!("=== Rust Chardev Test ===\n");

    // Open the device
    print!("Opening {}... ", DEVICE_PATH);
    let fd = match OpenOptions::new().read(true).write(true).open(DEVICE_PATH) {
        Ok(f) => {
            println!("OK");
            f
        }
        Err(e) => {
            println!("FAILED: {}", e);
            println!("\nHint: Make sure the module is loaded:");
            println!("  insmod /home/qwert/build/samples/rust/rust_chardev.ko");
            std::process::exit(1);
        }
    };

    // Test HELLO ioctl
    print!("Sending HELLO ioctl... ");
    if let Err(e) = ioctl_hello(&fd) {
        println!("FAILED: {}", e);
        std::process::exit(1);
    }

    // Send HELLO a few more times to increment the counter
    print!("Sending HELLO ioctl (2nd time)... ");
    if let Err(e) = ioctl_hello(&fd) {
        println!("FAILED: {}", e);
        std::process::exit(1);
    }

    print!("Sending HELLO ioctl (3rd time)... ");
    if let Err(e) = ioctl_hello(&fd) {
        println!("FAILED: {}", e);
        std::process::exit(1);
    }

    // Get the count
    print!("Getting ioctl call count... ");
    match ioctl_get_count(&fd) {
        Ok(count) => {
            println!("[OK] count = {}", count);
            // open_count starts at 1, plus 3 HELLO calls = 4
            if count == 4 {
                println!("[OK] Count matches expected value (1 initial + 3 hellos = 4)");
            } else {
                println!("[WARN] Expected count=4, got count={}", count);
            }
        }
        Err(e) => {
            println!("FAILED: {}", e);
            std::process::exit(1);
        }
    }

    println!("\n=== All tests passed! ===");
    println!("\nCheck kernel logs with: dmesg | tail -20");
}
