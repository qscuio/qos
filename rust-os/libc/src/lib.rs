#![no_std]

use core::ffi::{c_char, c_int, c_long, c_void};

#[cfg(target_os = "none")]
use core::sync::atomic::{AtomicI32, Ordering};

#[cfg(target_os = "none")]
static QOS_ERRNO: AtomicI32 = AtomicI32::new(0);

#[cfg(target_os = "none")]
unsafe extern "C" {
    fn qos_syscall_dispatch(nr: u32, a0: u64, a1: u64, a2: u64, a3: u64) -> i64;
}

#[cfg(target_os = "none")]
const SYSCALL_NR_CLOSE: u32 = 8;
#[cfg(target_os = "none")]
const SYSCALL_NR_SOCKET: u32 = 14;
#[cfg(target_os = "none")]
const SYSCALL_NR_BIND: u32 = 15;
#[cfg(target_os = "none")]
const SYSCALL_NR_LISTEN: u32 = 16;
#[cfg(target_os = "none")]
const SYSCALL_NR_ACCEPT: u32 = 17;
#[cfg(target_os = "none")]
const SYSCALL_NR_CONNECT: u32 = 18;
#[cfg(target_os = "none")]
const SYSCALL_NR_SEND: u32 = 19;
#[cfg(target_os = "none")]
const SYSCALL_NR_RECV: u32 = 20;
#[cfg(target_os = "none")]
const SYSCALL_NR_DLOPEN: u32 = 36;
#[cfg(target_os = "none")]
const SYSCALL_NR_DLCLOSE: u32 = 37;
#[cfg(target_os = "none")]
const SYSCALL_NR_DLSYM: u32 = 38;
#[cfg(target_os = "none")]
const SYSCALL_NR_MODLOAD: u32 = 39;
#[cfg(target_os = "none")]
const SYSCALL_NR_MODUNLOAD: u32 = 40;
#[cfg(target_os = "none")]
const SYSCALL_NR_MODRELOAD: u32 = 41;

#[cfg(target_os = "none")]
#[inline]
unsafe fn qos_syscall4(nr: u32, a0: u64, a1: u64, a2: u64, a3: u64) -> i64 {
    // SAFETY: Kernel entry point handles argument validation and returns i64 status.
    unsafe { qos_syscall_dispatch(nr, a0, a1, a2, a3) }
}

pub type QosSignalHandler = Option<extern "C" fn(c_int)>;

#[cfg(not(target_os = "none"))]
unsafe extern "C" {
    fn __errno_location() -> *mut c_int;
    fn malloc(size: usize) -> *mut c_void;
    fn printf(fmt: *const c_char, ...) -> c_int;
    fn snprintf(dst: *mut c_char, len: usize, fmt: *const c_char, ...) -> c_int;
    fn perror(prefix: *const c_char);
    fn free(ptr: *mut c_void);
    fn realloc(ptr: *mut c_void, size: usize) -> *mut c_void;
    fn strtok(s: *mut c_char, delim: *const c_char) -> *mut c_char;
    fn putchar(c: c_int) -> c_int;
    fn puts(s: *const c_char) -> c_int;
    fn getchar() -> c_int;
    fn fgets(s: *mut c_char, size: c_int, stream: *mut c_void) -> *mut c_char;
    fn fputs(s: *const c_char, stream: *mut c_void) -> c_int;
    fn fopen(path: *const c_char, mode: *const c_char) -> *mut c_void;
    fn fclose(stream: *mut c_void) -> c_int;
    fn fread(ptr: *mut c_void, size: usize, nmemb: usize, stream: *mut c_void) -> usize;
    fn fwrite(ptr: *const c_void, size: usize, nmemb: usize, stream: *mut c_void) -> usize;
    fn fseek(stream: *mut c_void, offset: c_long, whence: c_int) -> c_int;
    fn ftell(stream: *mut c_void) -> c_long;
    fn fflush(stream: *mut c_void) -> c_int;
    fn fileno(stream: *mut c_void) -> c_int;
    fn exit(status: c_int) -> !;
    fn _exit(status: c_int) -> !;
    fn getpid() -> c_int;
    fn fork() -> c_int;
    fn execv(path: *const c_char, argv: *const *const c_char) -> c_int;
    fn execve(path: *const c_char, argv: *const *const c_char, envp: *const *const c_char) -> c_int;
    fn waitpid(pid: c_int, status: *mut c_int, options: c_int) -> c_int;
    fn abort() -> !;
    fn socket(domain: c_int, sock_type: c_int, protocol: c_int) -> c_int;
    fn bind(fd: c_int, addr: *const c_void, addrlen: u32) -> c_int;
    fn listen(fd: c_int, backlog: c_int) -> c_int;
    fn accept(fd: c_int, addr: *mut c_void, addrlen: *mut u32) -> c_int;
    fn connect(fd: c_int, addr: *const c_void, addrlen: u32) -> c_int;
    fn send(fd: c_int, buf: *const c_void, len: usize, flags: c_int) -> isize;
    fn recv(fd: c_int, buf: *mut c_void, len: usize, flags: c_int) -> isize;
    fn sendto(
        fd: c_int,
        buf: *const c_void,
        len: usize,
        flags: c_int,
        addr: *const c_void,
        addrlen: u32,
    ) -> isize;
    fn recvfrom(
        fd: c_int,
        buf: *mut c_void,
        len: usize,
        flags: c_int,
        addr: *mut c_void,
        addrlen: *mut u32,
    ) -> isize;
    fn close(fd: c_int) -> c_int;
    fn kill(pid: c_int, signum: c_int) -> c_int;
    fn signal(signum: c_int, handler: QosSignalHandler) -> QosSignalHandler;
    fn sigaction(signum: c_int, act: *const c_void, oldact: *mut c_void) -> c_int;
    fn sigprocmask(how: c_int, set: *const c_void, oldset: *mut c_void) -> c_int;
    fn sigpending(set: *mut c_void) -> c_int;
    fn sigsuspend(mask: *const c_void) -> c_int;
    fn sigaltstack(ss: *const c_void, old_ss: *mut c_void) -> c_int;
    fn raise(signum: c_int) -> c_int;
}

pub fn qos_strlen(s: &[u8]) -> usize {
    let mut n = 0usize;
    while n < s.len() && s[n] != 0 {
        n += 1;
    }
    n
}

pub fn qos_strcmp(a: &[u8], b: &[u8]) -> i32 {
    let mut i = 0usize;
    loop {
        let ca = if i < a.len() { a[i] } else { 0 };
        let cb = if i < b.len() { b[i] } else { 0 };

        if ca != cb {
            return (ca as i32) - (cb as i32);
        }

        if ca == 0 {
            return 0;
        }

        i += 1;
    }
}

pub fn qos_strncmp(a: &[u8], b: &[u8], n: usize) -> i32 {
    let mut i = 0usize;
    while i < n {
        let ca = if i < a.len() { a[i] } else { 0 };
        let cb = if i < b.len() { b[i] } else { 0 };

        if ca != cb {
            return (ca as i32) - (cb as i32);
        }

        if ca == 0 {
            return 0;
        }

        i += 1;
    }

    0
}

pub fn qos_strcpy(dst: &mut [u8], src: &[u8]) {
    if dst.is_empty() {
        return;
    }
    let mut i = 0usize;
    while i + 1 < dst.len() && i < src.len() && src[i] != 0 {
        dst[i] = src[i];
        i += 1;
    }
    dst[i] = 0;
}

pub fn qos_strncpy(dst: &mut [u8], src: &[u8], n: usize) {
    if dst.is_empty() {
        return;
    }
    let max_copy = core::cmp::min(n, dst.len());
    let mut i = 0usize;
    while i < max_copy && i < src.len() && src[i] != 0 {
        dst[i] = src[i];
        i += 1;
    }
    while i < max_copy {
        dst[i] = 0;
        i += 1;
    }
}

pub fn qos_strchr(s: &[u8], c: u8) -> Option<usize> {
    let mut i = 0usize;
    while i < s.len() {
        if s[i] == c {
            return Some(i);
        }
        if s[i] == 0 {
            break;
        }
        i += 1;
    }
    None
}

pub fn qos_strrchr(s: &[u8], c: u8) -> Option<usize> {
    let mut i = 0usize;
    let mut last = None;
    while i < s.len() {
        if s[i] == c {
            last = Some(i);
        }
        if s[i] == 0 {
            break;
        }
        i += 1;
    }
    last
}

pub fn qos_atoi(s: &[u8]) -> i32 {
    let mut i = 0usize;
    let mut sign = 1i32;
    let mut value = 0i32;

    while i < s.len() && (s[i] == b' ' || s[i] == b'\t' || s[i] == b'\n' || s[i] == b'\r') {
        i += 1;
    }

    if i < s.len() && (s[i] == b'+' || s[i] == b'-') {
        if s[i] == b'-' {
            sign = -1;
        }
        i += 1;
    }

    while i < s.len() && s[i].is_ascii_digit() {
        value = value.saturating_mul(10).saturating_add((s[i] - b'0') as i32);
        i += 1;
    }

    sign.saturating_mul(value)
}

pub fn qos_htons(v: u16) -> u16 {
    v.swap_bytes()
}

pub fn qos_htonl(v: u32) -> u32 {
    v.swap_bytes()
}

pub fn qos_ntohs(v: u16) -> u16 {
    qos_htons(v)
}

pub fn qos_ntohl(v: u32) -> u32 {
    qos_htonl(v)
}

pub fn qos_inet_addr(s: &[u8]) -> Option<u32> {
    let mut parts = [0u32; 4];
    let mut idx = 0usize;
    let mut i = 0usize;

    while i < s.len() && s[i] != 0 {
        if idx >= 4 || !s[i].is_ascii_digit() {
            return None;
        }
        let mut value = 0u32;
        while i < s.len() && s[i].is_ascii_digit() {
            value = value.saturating_mul(10).saturating_add((s[i] - b'0') as u32);
            if value > 255 {
                return None;
            }
            i += 1;
        }
        parts[idx] = value;
        idx += 1;
        if i < s.len() && s[i] == b'.' {
            i += 1;
        } else if i < s.len() && s[i] != 0 {
            return None;
        }
    }

    if idx != 4 {
        return None;
    }
    Some((parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3])
}

fn write_u8_dec(value: u8, out: &mut [u8]) -> usize {
    if out.is_empty() {
        return 0;
    }
    if value >= 100 {
        if out.len() < 3 {
            return 0;
        }
        out[0] = b'0' + (value / 100);
        out[1] = b'0' + ((value / 10) % 10);
        out[2] = b'0' + (value % 10);
        3
    } else if value >= 10 {
        if out.len() < 2 {
            return 0;
        }
        out[0] = b'0' + (value / 10);
        out[1] = b'0' + (value % 10);
        2
    } else {
        out[0] = b'0' + value;
        1
    }
}

pub fn qos_inet_ntoa(addr: u32, out: &mut [u8]) -> Option<usize> {
    let octets = [
        ((addr >> 24) & 0xFF) as u8,
        ((addr >> 16) & 0xFF) as u8,
        ((addr >> 8) & 0xFF) as u8,
        (addr & 0xFF) as u8,
    ];
    let mut pos = 0usize;
    let mut i = 0usize;
    if out.is_empty() {
        return None;
    }
    while i < 4 {
        let written = write_u8_dec(octets[i], &mut out[pos..]);
        if written == 0 {
            return None;
        }
        pos += written;
        if i != 3 {
            if pos >= out.len() {
                return None;
            }
            out[pos] = b'.';
            pos += 1;
        }
        i += 1;
    }
    if pos >= out.len() {
        return None;
    }
    out[pos] = 0;
    Some(pos)
}

pub fn qos_errno_set(err: i32) {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: libc provides a thread-local errno pointer for the current thread.
        unsafe {
            *__errno_location() = err;
        }
    }
    #[cfg(target_os = "none")]
    {
        QOS_ERRNO.store(err, Ordering::SeqCst);
    }
}

pub fn qos_errno_get() -> i32 {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: libc provides a thread-local errno pointer for the current thread.
        unsafe { *__errno_location() }
    }
    #[cfg(target_os = "none")]
    {
        QOS_ERRNO.load(Ordering::SeqCst)
    }
}

pub fn qos_strerror(err: i32) -> &'static [u8] {
    match err {
        0 => b"Success",
        2 => b"No such file or directory",
        9 => b"Bad file descriptor",
        11 => b"Resource temporarily unavailable",
        12 => b"Out of memory",
        13 => b"Permission denied",
        17 => b"File exists",
        20 => b"Not a directory",
        21 => b"Is a directory",
        22 => b"Invalid argument",
        32 => b"Broken pipe",
        110 => b"Connection timed out",
        111 => b"Connection refused",
        _ => b"Unknown error",
    }
}

pub fn qos_getpid() -> i32 {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Direct host libc bridge for userspace/test builds.
        return unsafe { getpid() };
    }
    #[cfg(target_os = "none")]
    {
        1
    }
}

pub fn qos_memset(dst: &mut [u8], value: u8) {
    for b in dst.iter_mut() {
        *b = value;
    }
}

pub fn qos_memcmp(a: &[u8], b: &[u8]) -> i32 {
    let len = core::cmp::min(a.len(), b.len());
    let mut i = 0usize;
    while i < len {
        if a[i] != b[i] {
            return (a[i] as i32) - (b[i] as i32);
        }
        i += 1;
    }

    if a.len() == b.len() {
        0
    } else if a.len() < b.len() {
        -1
    } else {
        1
    }
}

pub fn qos_memcpy(dst: &mut [u8], src: &[u8]) {
    let len = core::cmp::min(dst.len(), src.len());
    dst[..len].copy_from_slice(&src[..len]);
}

pub fn qos_memmove(dst: &mut [u8], src: &[u8]) {
    let len = core::cmp::min(dst.len(), src.len());
    if len == 0 {
        return;
    }

    let dst_ptr = dst.as_mut_ptr() as usize;
    let src_ptr = src.as_ptr() as usize;

    if dst_ptr <= src_ptr || dst_ptr >= src_ptr + len {
        let mut i = 0usize;
        while i < len {
            dst[i] = src[i];
            i += 1;
        }
    } else {
        let mut i = len;
        while i > 0 {
            i -= 1;
            dst[i] = src[i];
        }
    }
}

pub unsafe fn qos_malloc(size: usize) -> *mut c_void {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller upholds allocation API contract.
        return unsafe { malloc(size) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = size;
        core::ptr::null_mut()
    }
}

pub unsafe fn qos_free(ptr: *mut c_void) {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller passes pointer previously returned by allocator.
        unsafe { free(ptr) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = ptr;
    }
}

pub unsafe fn qos_realloc(ptr: *mut c_void, size: usize) -> *mut c_void {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller upholds reallocation API contract.
        return unsafe { realloc(ptr, size) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = ptr;
        let _ = size;
        core::ptr::null_mut()
    }
}

pub unsafe fn qos_strtok(s: *mut c_char, delim: *const c_char) -> *mut c_char {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid mutable C string and delimiter.
        return unsafe { strtok(s, delim) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = s;
        let _ = delim;
        core::ptr::null_mut()
    }
}

pub unsafe fn qos_putchar(c: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call.
        return unsafe { putchar(c) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = c;
        -1
    }
}

pub unsafe fn qos_printf(fmt: *const c_char) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid format C string (no variadic args in this wrapper).
        return unsafe { printf(fmt) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = fmt;
        -1
    }
}

pub unsafe fn qos_snprintf(dst: *mut c_char, len: usize, fmt: *const c_char) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid output buffer and format C string.
        return unsafe { snprintf(dst, len, fmt) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = dst;
        let _ = len;
        let _ = fmt;
        -1
    }
}

pub unsafe fn qos_vsnprintf(dst: *mut c_char, len: usize, fmt: *const c_char, ap: *mut c_void) -> c_int {
    let _ = ap;
    // Reuse snprintf path because Rust stable does not support defining C-variadic wrappers.
    unsafe { qos_snprintf(dst, len, fmt) }
}

pub unsafe fn qos_puts(s: *const c_char) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid NUL-terminated string.
        return unsafe { puts(s) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = s;
        -1
    }
}

pub unsafe fn qos_getchar() -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call.
        return unsafe { getchar() };
    }
    #[cfg(target_os = "none")]
    {
        -1
    }
}

pub unsafe fn qos_fgets(s: *mut c_char, size: c_int, stream: *mut c_void) -> *mut c_char {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller passes valid pointers and buffer size.
        return unsafe { fgets(s, size, stream) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = s;
        let _ = size;
        let _ = stream;
        core::ptr::null_mut()
    }
}

pub unsafe fn qos_fputs(s: *const c_char, stream: *mut c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid C string and stream.
        return unsafe { fputs(s, stream) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = s;
        let _ = stream;
        -1
    }
}

pub unsafe fn qos_fopen(path: *const c_char, mode: *const c_char) -> *mut c_void {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid C strings.
        return unsafe { fopen(path, mode) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = path;
        let _ = mode;
        core::ptr::null_mut()
    }
}

pub unsafe fn qos_fclose(stream: *mut c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid stream pointer.
        return unsafe { fclose(stream) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = stream;
        -1
    }
}

pub unsafe fn qos_fread(ptr: *mut c_void, size: usize, nmemb: usize, stream: *mut c_void) -> usize {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid buffer and stream.
        return unsafe { fread(ptr, size, nmemb, stream) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = ptr;
        let _ = size;
        let _ = nmemb;
        let _ = stream;
        0
    }
}

pub unsafe fn qos_fwrite(
    ptr: *const c_void,
    size: usize,
    nmemb: usize,
    stream: *mut c_void,
) -> usize {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid buffer and stream.
        return unsafe { fwrite(ptr, size, nmemb, stream) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = ptr;
        let _ = size;
        let _ = nmemb;
        let _ = stream;
        0
    }
}

pub unsafe fn qos_fseek(stream: *mut c_void, offset: i64, whence: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid stream and offset.
        return unsafe { fseek(stream, offset as c_long, whence) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = stream;
        let _ = offset;
        let _ = whence;
        -1
    }
}

pub unsafe fn qos_ftell(stream: *mut c_void) -> i64 {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid stream.
        return unsafe { ftell(stream) as i64 };
    }
    #[cfg(target_os = "none")]
    {
        let _ = stream;
        -1
    }
}

pub unsafe fn qos_fflush(stream: *mut c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid stream.
        return unsafe { fflush(stream) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = stream;
        -1
    }
}

pub unsafe fn qos_fileno(stream: *mut c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid stream.
        return unsafe { fileno(stream) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = stream;
        -1
    }
}

pub unsafe fn qos_exit(status: c_int) -> ! {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Terminates process via host libc.
        unsafe { exit(status) }
    }
    #[cfg(target_os = "none")]
    {
        let _ = status;
        loop {
            core::hint::spin_loop();
        }
    }
}

#[allow(non_snake_case)]
pub unsafe fn qos__exit(status: c_int) -> ! {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Terminates process via host libc.
        unsafe { _exit(status) }
    }
    #[cfg(target_os = "none")]
    {
        let _ = status;
        loop {
            core::hint::spin_loop();
        }
    }
}

pub unsafe fn qos_fork() -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call.
        return unsafe { fork() };
    }
    #[cfg(target_os = "none")]
    {
        -1
    }
}

pub unsafe fn qos_execv(path: *const c_char, argv: *const *const c_char) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid exec path and argv pointers.
        return unsafe { execv(path, argv) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = path;
        let _ = argv;
        -1
    }
}

pub unsafe fn qos_execve(path: *const c_char, argv: *const *const c_char, envp: *const *const c_char) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid exec path, argv and envp pointers.
        return unsafe { execve(path, argv, envp) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = path;
        let _ = argv;
        let _ = envp;
        -1
    }
}

pub unsafe fn qos_waitpid(pid: c_int, status: *mut c_int, options: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid status pointer (or null) and options.
        return unsafe { waitpid(pid, status, options) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = pid;
        let _ = status;
        let _ = options;
        -1
    }
}

pub unsafe fn qos_dlopen(path: *const c_char, flags: c_int) -> *mut c_void {
    #[cfg(not(target_os = "none"))]
    {
        let _ = path;
        let _ = flags;
        core::ptr::null_mut()
    }
    #[cfg(target_os = "none")]
    {
        if path.is_null() {
            return core::ptr::null_mut();
        }
        let ret = unsafe { qos_syscall4(SYSCALL_NR_DLOPEN, path as u64, flags as u64, 0, 0) };
        if ret < 0 {
            core::ptr::null_mut()
        } else {
            ret as usize as *mut c_void
        }
    }
}

pub unsafe fn qos_dlclose(handle: *mut c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        let _ = handle;
        -1
    }
    #[cfg(target_os = "none")]
    {
        if handle.is_null() {
            return -1;
        }
        let ret = unsafe { qos_syscall4(SYSCALL_NR_DLCLOSE, handle as u64, 0, 0, 0) };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_dlsym(handle: *mut c_void, symbol: *const c_char) -> *mut c_void {
    #[cfg(not(target_os = "none"))]
    {
        let _ = handle;
        let _ = symbol;
        core::ptr::null_mut()
    }
    #[cfg(target_os = "none")]
    {
        if handle.is_null() || symbol.is_null() {
            return core::ptr::null_mut();
        }
        let ret = unsafe { qos_syscall4(SYSCALL_NR_DLSYM, handle as u64, symbol as u64, 0, 0) };
        if ret < 0 {
            core::ptr::null_mut()
        } else {
            ret as usize as *mut c_void
        }
    }
}

pub unsafe fn qos_modload(path: *const c_char) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        let _ = path;
        -1
    }
    #[cfg(target_os = "none")]
    {
        if path.is_null() {
            return -1;
        }
        let ret = unsafe { qos_syscall4(SYSCALL_NR_MODLOAD, path as u64, 0, 0, 0) };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_modunload(module_id: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        let _ = module_id;
        -1
    }
    #[cfg(target_os = "none")]
    {
        let ret = unsafe { qos_syscall4(SYSCALL_NR_MODUNLOAD, module_id as u64, 0, 0, 0) };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_modreload(module_id: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        let _ = module_id;
        -1
    }
    #[cfg(target_os = "none")]
    {
        let ret = unsafe { qos_syscall4(SYSCALL_NR_MODRELOAD, module_id as u64, 0, 0, 0) };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_abort() -> ! {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call that terminates process.
        unsafe { abort() }
    }
    #[cfg(target_os = "none")]
    {
        loop {
            core::hint::spin_loop();
        }
    }
}

pub unsafe fn qos_socket(domain: c_int, sock_type: c_int, protocol: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc socket syscall wrapper.
        return unsafe { socket(domain, sock_type, protocol) };
    }
    #[cfg(target_os = "none")]
    {
        let ret = unsafe {
            qos_syscall4(
                SYSCALL_NR_SOCKET,
                domain as u64,
                sock_type as u64,
                protocol as u64,
                0,
            )
        };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_bind(fd: c_int, addr: *const c_void, addrlen: u32) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid socket address pointer and length.
        return unsafe { bind(fd, addr, addrlen) };
    }
    #[cfg(target_os = "none")]
    {
        let ret = unsafe { qos_syscall4(SYSCALL_NR_BIND, fd as u64, addr as u64, addrlen as u64, 0) };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_listen(fd: c_int, backlog: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call.
        return unsafe { listen(fd, backlog) };
    }
    #[cfg(target_os = "none")]
    {
        let ret = unsafe { qos_syscall4(SYSCALL_NR_LISTEN, fd as u64, backlog as u64, 0, 0) };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_accept(fd: c_int, addr: *mut c_void, addrlen: *mut u32) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid optional address outputs.
        return unsafe { accept(fd, addr, addrlen) };
    }
    #[cfg(target_os = "none")]
    {
        let ret = unsafe { qos_syscall4(SYSCALL_NR_ACCEPT, fd as u64, addr as u64, addrlen as u64, 0) };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_connect(fd: c_int, addr: *const c_void, addrlen: u32) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid socket address pointer and length.
        return unsafe { connect(fd, addr, addrlen) };
    }
    #[cfg(target_os = "none")]
    {
        let ret = unsafe { qos_syscall4(SYSCALL_NR_CONNECT, fd as u64, addr as u64, addrlen as u64, 0) };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_send(fd: c_int, buf: *const c_void, len: usize, flags: c_int) -> isize {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid buffer for send length.
        return unsafe { send(fd, buf, len, flags) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = flags;
        let ret = unsafe { qos_syscall4(SYSCALL_NR_SEND, fd as u64, buf as u64, len as u64, 0) };
        if ret < 0 { -1 } else { ret as isize }
    }
}

pub unsafe fn qos_recv(fd: c_int, buf: *mut c_void, len: usize, flags: c_int) -> isize {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid writable buffer for recv length.
        return unsafe { recv(fd, buf, len, flags) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = flags;
        let ret = unsafe { qos_syscall4(SYSCALL_NR_RECV, fd as u64, buf as u64, len as u64, 0) };
        if ret < 0 { -1 } else { ret as isize }
    }
}

pub unsafe fn qos_sendto(
    fd: c_int,
    buf: *const c_void,
    len: usize,
    flags: c_int,
    addr: *const c_void,
    addrlen: u32,
) -> isize {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid buffer and destination address.
        return unsafe { sendto(fd, buf, len, flags, addr, addrlen) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = flags;
        let _ = addrlen;
        let ret = unsafe { qos_syscall4(SYSCALL_NR_SEND, fd as u64, buf as u64, len as u64, addr as u64) };
        if ret < 0 { -1 } else { ret as isize }
    }
}

pub unsafe fn qos_recvfrom(
    fd: c_int,
    buf: *mut c_void,
    len: usize,
    flags: c_int,
    addr: *mut c_void,
    addrlen: *mut u32,
) -> isize {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid writable buffer and optional source outputs.
        return unsafe { recvfrom(fd, buf, len, flags, addr, addrlen) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = flags;
        let ret = unsafe { qos_syscall4(SYSCALL_NR_RECV, fd as u64, buf as u64, len as u64, addr as u64) };
        if ret < 0 {
            -1
        } else {
            if !addrlen.is_null() {
                // Kernel recv path writes a fixed 16-byte sockaddr when source info is available.
                unsafe { *addrlen = 16 };
            }
            ret as isize
        }
    }
}

pub unsafe fn qos_close(fd: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call.
        return unsafe { close(fd) };
    }
    #[cfg(target_os = "none")]
    {
        let ret = unsafe { qos_syscall4(SYSCALL_NR_CLOSE, fd as u64, 0, 0, 0) };
        if ret < 0 { -1 } else { ret as c_int }
    }
}

pub unsafe fn qos_kill(pid: c_int, signum: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call.
        return unsafe { kill(pid, signum) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = pid;
        let _ = signum;
        -1
    }
}

pub unsafe fn qos_signal(signum: c_int, handler: QosSignalHandler) -> QosSignalHandler {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call with C ABI handler pointer.
        return unsafe { signal(signum, handler) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = signum;
        let _ = handler;
        None
    }
}

pub unsafe fn qos_sigaction(signum: c_int, act: *const c_void, oldact: *mut c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid optional sigaction structs.
        return unsafe { sigaction(signum, act, oldact) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = signum;
        let _ = act;
        let _ = oldact;
        -1
    }
}

pub unsafe fn qos_sigprocmask(how: c_int, set: *const c_void, oldset: *mut c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid optional sigset pointers.
        return unsafe { sigprocmask(how, set, oldset) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = how;
        let _ = set;
        let _ = oldset;
        -1
    }
}

pub unsafe fn qos_sigpending(set: *mut c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid sigset output pointer.
        return unsafe { sigpending(set) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = set;
        -1
    }
}

pub unsafe fn qos_sigsuspend(mask: *const c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid signal mask pointer.
        return unsafe { sigsuspend(mask) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = mask;
        -1
    }
}

pub unsafe fn qos_sigaltstack(ss: *const c_void, old_ss: *mut c_void) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Caller provides valid optional altstack pointers.
        return unsafe { sigaltstack(ss, old_ss) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = ss;
        let _ = old_ss;
        -1
    }
}

pub unsafe fn qos_raise(signum: c_int) -> c_int {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call.
        return unsafe { raise(signum) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = signum;
        -1
    }
}

pub unsafe fn qos_perror(prefix: *const c_char) {
    #[cfg(not(target_os = "none"))]
    {
        // SAFETY: Host libc call.
        unsafe { perror(prefix) };
    }
    #[cfg(target_os = "none")]
    {
        let _ = prefix;
    }
}
