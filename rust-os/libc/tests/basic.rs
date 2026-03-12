use qos_libc::{
    qos_atoi, qos_errno_get, qos_errno_set, qos_getpid, qos_htonl, qos_htons, qos_inet_addr,
    qos_inet_ntoa, qos_memcmp, qos_memcpy, qos_memmove, qos_memset, qos_ntohl, qos_ntohs,
    qos_strchr, qos_strcmp, qos_strcpy, qos_strerror, qos_strlen, qos_strncmp, qos_strncpy,
    qos_strrchr, qos__exit, qos_abort, qos_accept, qos_bind, qos_close, qos_connect, qos_execv,
    qos_execve, qos_exit, qos_fclose, qos_fflush, qos_fgets, qos_fileno, qos_fopen, qos_fork,
    qos_fputs, qos_fread, qos_free, qos_fseek, qos_ftell, qos_fwrite, qos_getchar, qos_kill,
    qos_listen, qos_malloc, qos_perror, qos_printf, qos_putchar, qos_puts, qos_raise, qos_realloc,
    qos_recv, qos_recvfrom, qos_send, qos_sendto, qos_sigaction, qos_sigaltstack, qos_sigpending,
    qos_sigprocmask, qos_signal, qos_sigsuspend, qos_snprintf, qos_socket, qos_strtok,
    qos_vsnprintf, qos_waitpid,
};
use core::ffi::c_char;
use std::ffi::{CStr, CString};
use std::thread;

#[test]
fn string_helpers_work() {
    assert_eq!(qos_strlen(b"qos"), 3);
    assert_eq!(qos_strcmp(b"abc", b"abc"), 0);
    assert!(qos_strcmp(b"abc", b"abd") < 0);
    assert!(qos_strcmp(b"abd", b"abc") > 0);
}

#[test]
fn memory_helpers_work() {
    let mut buf = [0_u8; 8];
    qos_memset(&mut buf, 0xAB);
    assert_eq!(buf, [0xAB; 8]);

    let src = [1_u8, 2, 3, 4, 5, 6, 7, 8];
    qos_memcpy(&mut buf, &src);
    assert_eq!(buf, src);
    assert_eq!(qos_memcmp(&buf, &src), 0);
    assert!(qos_memcmp(&buf, &[1, 2, 3, 4, 5, 6, 7, 9]) < 0);

    let mut overlap = [1_u8, 2, 3, 4, 5, 6, 7, 8];
    let ptr = overlap.as_mut_ptr();
    // Exercise overlapping copy semantics explicitly.
    let src = unsafe { core::slice::from_raw_parts(ptr, 6) };
    let dst = unsafe { core::slice::from_raw_parts_mut(ptr.add(2), 6) };
    qos_memmove(dst, src);
    assert_eq!(overlap, [1, 2, 1, 2, 3, 4, 5, 6]);
}

#[test]
fn strncmp_helper_works() {
    assert_eq!(qos_strncmp(b"qos-shell", b"qos-sched", 5), 0);
    assert!(qos_strncmp(b"abc", b"abd", 3) < 0);
}

#[test]
fn extended_string_helpers_work() {
    let mut dst = [0u8; 16];
    qos_strcpy(&mut dst, b"hello\0");
    assert_eq!(&dst[..6], b"hello\0");

    let mut dst2 = [0u8; 8];
    qos_strncpy(&mut dst2, b"alphabet\0", 4);
    assert_eq!(&dst2[..5], b"alph\0");

    let mut untouched = *b"XYZ";
    qos_strncpy(&mut untouched, b"abc\0", 0);
    assert_eq!(&untouched, b"XYZ");

    assert_eq!(qos_strchr(b"qos-shell\0", b's'), Some(2));
    assert_eq!(qos_strrchr(b"qos-shell\0", b's'), Some(4));
    assert_eq!(qos_strchr(b"qos-shell\0", b'z'), None);
}

#[test]
fn atoi_helper_works() {
    assert_eq!(qos_atoi(b"0\0"), 0);
    assert_eq!(qos_atoi(b"42\0"), 42);
    assert_eq!(qos_atoi(b"-17\0"), -17);
    assert_eq!(qos_atoi(b"  9abc\0"), 9);
}

#[test]
fn byte_order_helpers_work() {
    assert_eq!(qos_htons(0x1234), 0x3412);
    assert_eq!(qos_ntohs(0x3412), 0x1234);
    assert_eq!(qos_htonl(0x01020304), 0x04030201);
    assert_eq!(qos_ntohl(0x04030201), 0x01020304);
}

#[test]
fn inet_helpers_work() {
    assert_eq!(qos_inet_addr(b"10.0.2.15\0"), Some(0x0A00020F));
    assert_eq!(qos_inet_addr(b"300.0.0.1\0"), None);

    let mut out = [0u8; 32];
    let len = qos_inet_ntoa(0xC0A8_0101, &mut out).expect("inet_ntoa");
    assert_eq!(&out[..len], b"192.168.1.1");
    assert_eq!(out[len], 0);
}

#[test]
fn errno_and_pid_helpers_work() {
    qos_errno_set(111);
    assert_eq!(qos_errno_get(), 111);
    assert_eq!(qos_strerror(111), b"Connection refused");
    assert!(qos_getpid() > 0);
}

#[test]
fn errno_is_thread_local() {
    qos_errno_set(7);
    let t1 = thread::spawn(|| {
        qos_errno_set(11);
        qos_errno_get()
    });
    let t2 = thread::spawn(|| {
        qos_errno_set(22);
        qos_errno_get()
    });
    let a = t1.join().expect("t1");
    let b = t2.join().expect("t2");
    assert_eq!((a, b), (11, 22));
    assert_eq!(qos_errno_get(), 7);
}

#[test]
fn host_wrapper_exports_and_smoke_work() {
    let _ = qos_putchar as unsafe fn(i32) -> i32;
    let _ = qos_puts as unsafe fn(*const c_char) -> i32;
    let _ = qos_getchar as unsafe fn() -> i32;
    let _ = qos_fgets as unsafe fn(*mut c_char, i32, *mut core::ffi::c_void) -> *mut c_char;
    let _ = qos_fputs as unsafe fn(*const c_char, *mut core::ffi::c_void) -> i32;
    let _ = qos_printf as unsafe fn(*const c_char) -> i32;
    let _ = qos_snprintf as unsafe fn(*mut c_char, usize, *const c_char) -> i32;
    let _ = qos_vsnprintf as unsafe fn(*mut c_char, usize, *const c_char, *mut core::ffi::c_void) -> i32;
    let _ = qos_perror as unsafe fn(*const c_char);
    let _ = qos_fread as unsafe fn(*mut core::ffi::c_void, usize, usize, *mut core::ffi::c_void) -> usize;
    let _ = qos_fseek as unsafe fn(*mut core::ffi::c_void, i64, i32) -> i32;
    let _ = qos_ftell as unsafe fn(*mut core::ffi::c_void) -> i64;
    let _ = qos_fileno as unsafe fn(*mut core::ffi::c_void) -> i32;
    let _ = qos_exit as unsafe fn(i32) -> !;
    let _ = qos__exit as unsafe fn(i32) -> !;
    let _ = qos_fork as unsafe fn() -> i32;
    let _ = qos_execv as unsafe fn(*const c_char, *const *const c_char) -> i32;
    let _ = qos_execve as unsafe fn(*const c_char, *const *const c_char, *const *const c_char) -> i32;
    let _ = qos_waitpid as unsafe fn(i32, *mut i32, i32) -> i32;
    let _ = qos_abort as unsafe fn() -> !;
    let _ = qos_bind as unsafe fn(i32, *const core::ffi::c_void, u32) -> i32;
    let _ = qos_listen as unsafe fn(i32, i32) -> i32;
    let _ = qos_accept as unsafe fn(i32, *mut core::ffi::c_void, *mut u32) -> i32;
    let _ = qos_connect as unsafe fn(i32, *const core::ffi::c_void, u32) -> i32;
    let _ = qos_send as unsafe fn(i32, *const core::ffi::c_void, usize, i32) -> isize;
    let _ = qos_recv as unsafe fn(i32, *mut core::ffi::c_void, usize, i32) -> isize;
    let _ = qos_sendto as unsafe fn(i32, *const core::ffi::c_void, usize, i32, *const core::ffi::c_void, u32) -> isize;
    let _ = qos_recvfrom as unsafe fn(i32, *mut core::ffi::c_void, usize, i32, *mut core::ffi::c_void, *mut u32) -> isize;
    let _ = qos_kill as unsafe fn(i32, i32) -> i32;
    let _ = qos_signal as unsafe fn(i32, Option<extern "C" fn(i32)>) -> Option<extern "C" fn(i32)>;
    let _ = qos_sigaction as unsafe fn(i32, *const core::ffi::c_void, *mut core::ffi::c_void) -> i32;
    let _ = qos_sigprocmask as unsafe fn(i32, *const core::ffi::c_void, *mut core::ffi::c_void) -> i32;
    let _ = qos_sigpending as unsafe fn(*mut core::ffi::c_void) -> i32;
    let _ = qos_sigsuspend as unsafe fn(*const core::ffi::c_void) -> i32;
    let _ = qos_sigaltstack as unsafe fn(*const core::ffi::c_void, *mut core::ffi::c_void) -> i32;
    let _ = qos_raise as unsafe fn(i32) -> i32;

    unsafe {
        let ptr = qos_malloc(16);
        assert!(!ptr.is_null());
        let ptr2 = qos_realloc(ptr, 64);
        assert!(!ptr2.is_null());
        qos_free(ptr2);

        let mut tokens = b"aa,bb,cc\0".to_vec();
        let comma = CString::new(",").expect("comma cstring");
        let first = qos_strtok(tokens.as_mut_ptr().cast(), comma.as_ptr().cast());
        assert_eq!(CStr::from_ptr(first).to_bytes(), b"aa");
        let second = qos_strtok(core::ptr::null_mut(), comma.as_ptr().cast());
        assert_eq!(CStr::from_ptr(second).to_bytes(), b"bb");
        let third = qos_strtok(core::ptr::null_mut(), comma.as_ptr().cast());
        assert_eq!(CStr::from_ptr(third).to_bytes(), b"cc");
        assert!(qos_strtok(core::ptr::null_mut(), comma.as_ptr().cast()).is_null());

        let fd = qos_socket(2, 1, 0);
        assert!(fd >= 0);
        assert_eq!(qos_close(fd), 0);

        let mut out = [0 as c_char; 16];
        let literal = CString::new("qos").expect("literal cstring");
        assert_eq!(qos_snprintf(out.as_mut_ptr(), out.len(), literal.as_ptr().cast()), 3);
        assert_eq!(CStr::from_ptr(out.as_ptr()).to_bytes(), b"qos");
        assert_eq!(
            qos_vsnprintf(out.as_mut_ptr(), out.len(), literal.as_ptr().cast(), core::ptr::null_mut()),
            3
        );

        let path = std::env::temp_dir().join(format!("qos-libc-{}.txt", qos_getpid()));
        let path_c = CString::new(path.to_string_lossy().as_bytes()).expect("path cstring");
        let mode = CString::new("wb").expect("mode cstring");
        let file = qos_fopen(path_c.as_ptr().cast(), mode.as_ptr().cast());
        assert!(!file.is_null());
        let payload = b"qos";
        assert_eq!(qos_fwrite(payload.as_ptr().cast(), 1, payload.len(), file), 3);
        assert_eq!(qos_fflush(file), 0);
        assert_eq!(qos_fclose(file), 0);
        std::fs::remove_file(path).ok();
    }
}
