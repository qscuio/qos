// SPDX-License-Identifier: GPL-2.0
//
// Userspace smoke test for the rust_workqueue sample.

use std::fs::OpenOptions;
use std::io::{self, Write};
use std::os::unix::io::AsRawFd;

const DEVICE_PATH: &str = "/dev/rust-workqueue";
const GET_COMPLETED: libc::c_ulong = 0x8008_51a0;

#[allow(non_camel_case_types)]
mod libc {
    pub type c_ulong = u64;
    pub type c_int = i32;

    extern "C" {
        pub fn ioctl(fd: c_int, request: c_ulong, ...) -> c_int;
    }
}

fn ioctl_completed(fd: i32, completed: &mut u64) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, GET_COMPLETED, completed as *mut u64) };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

fn main() {
    let mut file = match OpenOptions::new().read(true).write(true).open(DEVICE_PATH) {
        Ok(file) => file,
        Err(err) => {
            eprintln!("open {} failed: {}", DEVICE_PATH, err);
            std::process::exit(1);
        }
    };

    for payload in ["first work item\n", "second work item\n"] {
        if let Err(err) = file.write_all(payload.as_bytes()) {
            eprintln!("write failed: {}", err);
            std::process::exit(1);
        }
    }

    let mut completed = 0_u64;
    if let Err(err) = ioctl_completed(file.as_raw_fd(), &mut completed) {
        eprintln!("GET_COMPLETED failed: {}", err);
        std::process::exit(1);
    }

    println!("completed={}", completed);
}
