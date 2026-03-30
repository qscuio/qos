// SPDX-License-Identifier: GPL-2.0
//
// Userspace smoke test for the rust_platform sample.

use std::fs::OpenOptions;
use std::io;
use std::os::unix::io::AsRawFd;

const DEVICE_PATH: &str = "/dev/rust-platform";
const REG_READ: libc::c_ulong = 0xc008_51b0;
const REG_WRITE: libc::c_ulong = 0x4008_51b1;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct RegisterValue {
    index: u32,
    value: u32,
}

#[allow(non_camel_case_types)]
mod libc {
    pub type c_ulong = u64;
    pub type c_int = i32;

    extern "C" {
        pub fn ioctl(fd: c_int, request: c_ulong, ...) -> c_int;
    }
}

fn ioctl_reg_write(fd: i32, arg: &RegisterValue) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, REG_WRITE, arg as *const RegisterValue) };
    if ret < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

fn ioctl_reg_read(fd: i32, arg: &mut RegisterValue) -> io::Result<()> {
    let ret = unsafe { libc::ioctl(fd, REG_READ, arg as *mut RegisterValue) };
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
    let write_arg = RegisterValue {
        index: 0,
        value: 0xfeed_cafe,
    };
    if let Err(err) = ioctl_reg_write(fd, &write_arg) {
        eprintln!("REG_WRITE failed: {}", err);
        std::process::exit(1);
    }

    let mut read_arg = RegisterValue { index: 0, value: 0 };
    if let Err(err) = ioctl_reg_read(fd, &mut read_arg) {
        eprintln!("REG_READ failed: {}", err);
        std::process::exit(1);
    }

    println!("register[{}]={:#x}", read_arg.index, read_arg.value);
}
