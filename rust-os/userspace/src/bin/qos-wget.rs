use qos_userspace::net::{ensure_kernel_booted, wget_via_syscall_stream};

fn main() {
    ensure_kernel_booted();
    let url = match std::env::args().nth(1) {
        Some(v) => v,
        None => {
            println!("wget: missing url");
            return;
        }
    };
    if let Some(resp) = wget_via_syscall_stream(&url) {
        print!("{resp}");
    } else {
        println!("wget: failed");
    }
}
