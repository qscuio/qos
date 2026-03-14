// SPDX-License-Identifier: GPL-2.0

//! Rust character device - A learning module demonstrating misc device usage.
//!
//! This module demonstrates:
//! - Registering a misc character device
//! - Handling ioctl commands from userspace
//! - Mutex-protected shared state
//! - User-kernel data transfer with `UserSlice`
//! - The `InPlaceModule` trait and pin-init pattern
//!
//! Usage:
//!   insmod rust_chardev.ko
//!   # The device appears at /dev/rust-chardev
//!
//! Test with a C program:
//!   ```c
//!   #include <stdio.h>
//!   #include <fcntl.h>
//!   #include <unistd.h>
//!   #include <sys/ioctl.h>
//!
//!   #define CHARDEV_HELLO     _IO('Q', 0x80)
//!   #define CHARDEV_GET_COUNT _IOR('Q', 0x81, int)
//!
//!   int main() {
//!       int fd = open("/dev/rust-chardev", O_RDWR);
//!       if (fd < 0) { perror("open"); return 1; }
//!
//!       ioctl(fd, CHARDEV_HELLO, NULL);
//!
//!       int count;
//!       ioctl(fd, CHARDEV_GET_COUNT, &count);
//!       printf("Open count: %d\n", count);
//!
//!       close(fd);
//!       return 0;
//!   }
//!   ```

use core::pin::Pin;

use kernel::{
    c_str,
    device::Device,
    ioctl::{_IO, _IOC_SIZE, _IOR},
    miscdevice::{MiscDevice, MiscDeviceOptions, MiscDeviceRegistration},
    new_mutex,
    prelude::*,
    sync::{aref::ARef, Mutex},
    uaccess::{UserSlice, UserSliceWriter},
};

const CHARDEV_HELLO: u32 = _IO('Q' as u32, 0x80);
const CHARDEV_GET_COUNT: u32 = _IOR::<i32>('Q' as u32, 0x81);

module! {
    type: RustChardevModule,
    name: "rust_chardev",
    authors: ["Qulk Learning"],
    description: "Rust misc character device learning module",
    license: "GPL",
}

/// Module state - holds the misc device registration.
#[pin_data]
struct RustChardevModule {
    #[pin]
    _miscdev: MiscDeviceRegistration<RustChardev>,
}

impl kernel::InPlaceModule for RustChardevModule {
    fn init(_module: &'static ThisModule) -> impl PinInit<Self, Error> {
        pr_info!("Initialising Rust chardev learning module\n");

        let options = MiscDeviceOptions {
            name: c_str!("rust-chardev"),
        };

        try_pin_init!(Self {
            _miscdev <- MiscDeviceRegistration::register(options),
        })
    }
}

/// Per-open state - each open() gets its own instance.
#[pin_data(PinnedDrop)]
struct RustChardev {
    #[pin]
    inner: Mutex<ChardevInner>,
    dev: ARef<Device>,
}

struct ChardevInner {
    open_count: i32,
}

#[vtable]
impl MiscDevice for RustChardev {
    type Ptr = Pin<KBox<Self>>;

    fn open(_file: &kernel::fs::File, misc: &MiscDeviceRegistration<Self>) -> Result<Pin<KBox<Self>>> {
        let dev = ARef::from(misc.device());

        dev_info!(dev, "Device opened\n");

        KBox::try_pin_init(
            try_pin_init! {
                RustChardev {
                    inner <- new_mutex!(ChardevInner {
                        open_count: 1_i32,
                    }),
                    dev: dev,
                }
            },
            GFP_KERNEL,
        )
    }

    fn ioctl(me: Pin<&RustChardev>, _file: &kernel::fs::File, cmd: u32, arg: usize) -> Result<isize> {
        let arg = UserPtr::from_addr(arg);
        let size = _IOC_SIZE(cmd);

        match cmd {
            CHARDEV_HELLO => {
                dev_info!(me.dev, "Hello from Rust chardev ioctl!\n");
                let mut inner = me.inner.lock();
                inner.open_count += 1;
                dev_info!(me.dev, "Ioctl call count: {}\n", inner.open_count);
                Ok(0)
            }
            CHARDEV_GET_COUNT => {
                let inner = me.inner.lock();
                let count = inner.open_count;
                drop(inner);

                let mut writer: UserSliceWriter = UserSlice::new(arg, size).writer();
                writer.write::<i32>(&count)?;
                Ok(0)
            }
            _ => {
                dev_err!(me.dev, "Unknown ioctl command: {}\n", cmd);
                Err(ENOTTY)
            }
        }
    }
}

#[pinned_drop]
impl PinnedDrop for RustChardev {
    fn drop(self: Pin<&mut Self>) {
        dev_info!(self.dev, "Rust chardev closed\n");
    }
}
