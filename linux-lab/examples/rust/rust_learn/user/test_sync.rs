// SPDX-License-Identifier: GPL-2.0
//
// Userspace smoke test for the rust_sync sample.

use std::fs::OpenOptions;
use std::io;
use std::os::unix::io::AsRawFd;

const DEVICE_PATH: &str = "/dev/rust-sync";
const SYNC_PUSH: libc::c_ulong = 0x4008_5190;
const SYNC_POP: libc::c_ulong = 0x8008_5191;
const SYNC_STATS: libc::c_ulong = 0x8018_5192;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct SyncStats {
    push_count: u64,
    pop_count: u64,
    high_watermark: u32,
    depth: u32,
}

#[allow(non_camel_case_types)]
mod libc {
    pub type c_ulong = u64;
    pub type c_int = i32;

    extern "C" {
        pub fn ioctl(fd: c_int, request: c_ulong, ...) -> c_int;
    }
}

fn ioctl_push(fd: i32, value: &u64) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, SYNC_PUSH, value as *const u64) };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

fn ioctl_pop(fd: i32, value: &mut u64) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, SYNC_POP, value as *mut u64) };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

fn ioctl_stats(fd: i32, stats: &mut SyncStats) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, SYNC_STATS, stats as *mut SyncStats) };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

fn main() {
    let file = match OpenOptions::new().read(true).write(true).open(DEVICE_PATH) {
        Ok(file) => file,
        Err(err) => {
            eprintln!("open {} failed: {}", DEVICE_PATH, err);
            std::process::exit(1);
        }
    };

    let fd = file.as_raw_fd();
    for value in [11_u64, 22, 33] {
        if let Err(err) = ioctl_push(fd, &value) {
            eprintln!("push {} failed: {}", value, err);
            std::process::exit(1);
        }
    }

    let mut popped = 0_u64;
    if let Err(err) = ioctl_pop(fd, &mut popped) {
        eprintln!("pop failed: {}", err);
        std::process::exit(1);
    }

    let mut stats = SyncStats::default();
    if let Err(err) = ioctl_stats(fd, &mut stats) {
        eprintln!("stats failed: {}", err);
        std::process::exit(1);
    }

    println!("popped={}", popped);
    println!("stats={:?}", stats);
}
