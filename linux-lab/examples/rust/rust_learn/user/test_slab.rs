// SPDX-License-Identifier: GPL-2.0
//
// Userspace smoke test for the rust_slab sample.

use std::fs::OpenOptions;
use std::io;
use std::os::unix::io::AsRawFd;

const DEVICE_PATH: &str = "/dev/rust-slab";
const SLAB_ALLOC: libc::c_ulong = 0x8008_51c0;
const SLAB_FREE: libc::c_ulong = 0x4008_51c1;
const SLAB_FILL: libc::c_ulong = 0x4010_51c2;
const SLAB_STATS: libc::c_ulong = 0x800c_51c3;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct FillRequest {
    handle: u64,
    byte: u8,
    padding: [u8; 7],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct SlabStats {
    active: u32,
    total: u32,
    highwater: u32,
}

#[allow(non_camel_case_types)]
mod libc {
    pub type c_ulong = u64;
    pub type c_int = i32;

    extern "C" {
        pub fn ioctl(fd: c_int, request: c_ulong, ...) -> c_int;
    }
}

fn ioctl_alloc(fd: i32, handle: &mut u64) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, SLAB_ALLOC, handle as *mut u64) };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

fn ioctl_free(fd: i32, handle: &u64) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, SLAB_FREE, handle as *const u64) };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

fn ioctl_fill(fd: i32, request: &FillRequest) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, SLAB_FILL, request as *const FillRequest) };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

fn ioctl_stats(fd: i32, stats: &mut SlabStats) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, SLAB_STATS, stats as *mut SlabStats) };
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
    let mut handle = 0_u64;
    if let Err(err) = ioctl_alloc(fd, &mut handle) {
        eprintln!("ALLOC failed: {}", err);
        std::process::exit(1);
    }

    let fill = FillRequest {
        handle,
        byte: b'A',
        padding: [0; 7],
    };
    if let Err(err) = ioctl_fill(fd, &fill) {
        eprintln!("FILL failed: {}", err);
        std::process::exit(1);
    }

    let mut stats = SlabStats::default();
    if let Err(err) = ioctl_stats(fd, &mut stats) {
        eprintln!("STATS failed: {}", err);
        std::process::exit(1);
    }

    if let Err(err) = ioctl_free(fd, &handle) {
        eprintln!("FREE failed: {}", err);
        std::process::exit(1);
    }

    println!("handle={:#x}", handle);
    println!("stats={:?}", stats);
}
