#![no_std]

#[cfg(test)]
extern crate std;

use core::cell::UnsafeCell;
use core::ffi::CStr;
use core::hint::spin_loop;
use core::sync::atomic::{AtomicBool, AtomicU32, Ordering};
use qos_boot::boot_info::{BootInfo, QOS_BOOT_MAGIC, QOS_MMAP_MAX_ENTRIES};

pub const QOS_INIT_MM: u32 = 1 << 0;
pub const QOS_INIT_DRIVERS: u32 = 1 << 1;
pub const QOS_INIT_VFS: u32 = 1 << 2;
pub const QOS_INIT_NET: u32 = 1 << 3;
pub const QOS_INIT_SCHED: u32 = 1 << 4;
pub const QOS_INIT_SYSCALL: u32 = 1 << 5;
pub const QOS_INIT_PROC: u32 = 1 << 6;
pub const QOS_INIT_ALL: u32 =
    QOS_INIT_MM
        | QOS_INIT_DRIVERS
        | QOS_INIT_VFS
        | QOS_INIT_NET
        | QOS_INIT_SCHED
        | QOS_INIT_SYSCALL
        | QOS_INIT_PROC;

pub const QOS_PAGE_SIZE: u64 = 4096;
pub const QOS_PMM_MAX_PAGES: usize = 4096;
pub const QOS_VMM_MAX_MAPPINGS: usize = 2048;
pub const QOS_VMM_MAX_AS: usize = 64;

pub const VM_READ: u32 = 1 << 0;
pub const VM_WRITE: u32 = 1 << 1;
pub const VM_EXEC: u32 = 1 << 2;
pub const VM_USER: u32 = 1 << 3;
pub const VM_COW: u32 = 1 << 4;

static INIT_STATE: AtomicU32 = AtomicU32::new(0);
static MM_LOCK: AtomicBool = AtomicBool::new(false);
static SCHED_LOCK: AtomicBool = AtomicBool::new(false);
static VFS_LOCK: AtomicBool = AtomicBool::new(false);
static NET_LOCK: AtomicBool = AtomicBool::new(false);
static DRIVERS_LOCK: AtomicBool = AtomicBool::new(false);
static SYSCALL_LOCK: AtomicBool = AtomicBool::new(false);
static PROC_LOCK: AtomicBool = AtomicBool::new(false);
static SOFTIRQ_LOCK: AtomicBool = AtomicBool::new(false);
static TIMER_LOCK: AtomicBool = AtomicBool::new(false);
static KTHREAD_LOCK: AtomicBool = AtomicBool::new(false);
static NAPI_LOCK: AtomicBool = AtomicBool::new(false);
static WORKQUEUE_LOCK: AtomicBool = AtomicBool::new(false);

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum KernelError {
    InvalidBootInfo,
    InitIncomplete,
    InvalidAddress,
    VmmFull,
}

#[derive(Clone, Copy)]
struct PmmState {
    frames: [u64; QOS_PMM_MAX_PAGES],
    used: [u8; QOS_PMM_MAX_PAGES],
    frame_count: u32,
    free_count: u32,
}

impl PmmState {
    const fn new() -> Self {
        Self {
            frames: [0; QOS_PMM_MAX_PAGES],
            used: [0; QOS_PMM_MAX_PAGES],
            frame_count: 0,
            free_count: 0,
        }
    }
}

struct PmmCell(UnsafeCell<PmmState>);

// PMM state is guarded by MM_LOCK.
unsafe impl Sync for PmmCell {}

static PMM: PmmCell = PmmCell(UnsafeCell::new(PmmState::new()));

#[derive(Clone, Copy)]
struct VmmState {
    vas: [u64; QOS_VMM_MAX_MAPPINGS],
    pas: [u64; QOS_VMM_MAX_MAPPINGS],
    flags: [u32; QOS_VMM_MAX_MAPPINGS],
    asids: [u32; QOS_VMM_MAX_MAPPINGS],
    used: [u8; QOS_VMM_MAX_MAPPINGS],
    current_asid: u32,
}

impl VmmState {
    const fn new() -> Self {
        Self {
            vas: [0; QOS_VMM_MAX_MAPPINGS],
            pas: [0; QOS_VMM_MAX_MAPPINGS],
            flags: [0; QOS_VMM_MAX_MAPPINGS],
            asids: [0; QOS_VMM_MAX_MAPPINGS],
            used: [0; QOS_VMM_MAX_MAPPINGS],
            current_asid: 0,
        }
    }
}

struct VmmCell(UnsafeCell<VmmState>);

// VMM state is guarded by MM_LOCK.
unsafe impl Sync for VmmCell {}

static VMM: VmmCell = VmmCell(UnsafeCell::new(VmmState::new()));

pub const QOS_SCHED_MAX_TASKS: usize = 64;
pub const QOS_VFS_MAX_NODES: usize = 128;
pub const QOS_VFS_PATH_MAX: usize = 128;
pub const QOS_VFS_FS_INITRAMFS: u32 = 1;
pub const QOS_VFS_FS_TMPFS: u32 = 2;
pub const QOS_VFS_FS_PROCFS: u32 = 3;
pub const QOS_VFS_FS_EXT2: u32 = 4;
pub const QOS_NET_MAX_QUEUE: usize = 32;
pub const QOS_NET_MAX_PACKET: usize = 1500;
pub const QOS_NET_ARP_MAX: usize = 32;
pub const QOS_NET_UDP_PORT_MAX: usize = 64;
pub const QOS_NET_UDP_MAX_DATAGRAMS: usize = 64;
pub const QOS_NET_TCP_MAX_LISTENERS: usize = 64;
pub const QOS_NET_TCP_MAX_CONNS: usize = 128;
pub const QOS_NET_IPV4_LOCAL: u32 = 0x0A00020F;
pub const QOS_NET_IPV4_GATEWAY: u32 = 0x0A000202;
pub const QOS_NET_IPV4_SUBNET_MASK: u32 = 0xFFFFFF00;
pub const QOS_NET_PORT_MIN_DYNAMIC: u16 = 1024;
pub const QOS_DRIVERS_MAX: usize = 64;
pub const QOS_DRIVER_NAME_MAX: usize = 64;
pub const QOS_SYSCALL_MAX: usize = 128;
pub const QOS_FD_MAX: usize = 256;
pub const QOS_PIPE_MAX: usize = 32;
pub const QOS_PIPE_CAP: usize = 1024;
pub const QOS_PROC_MAX: usize = 256;
pub const QOS_PROC_NAME_MAX: usize = 32;
pub const QOS_SIGNAL_MAX: usize = 32;
pub const QOS_WAIT_WNOHANG: u32 = 1;
pub const QOS_SOFTIRQ_MAX: usize = 8;
pub const QOS_SOFTIRQ_TIMER: u32 = 0;
pub const QOS_SOFTIRQ_NET_RX: u32 = 1;
pub const QOS_SOFTIRQ_WORKQUEUE: u32 = 2;
pub const QOS_TIMER_MAX: usize = 128;
pub const QOS_KTHREAD_MAX: usize = 64;
pub const QOS_KTHREAD_NAME_MAX: usize = 32;
pub const QOS_NAPI_MAX: usize = 32;
pub const QOS_WORKQUEUE_MAX: usize = 128;

pub const QOS_SIG_DFL: u32 = 0;
pub const QOS_SIG_IGN: u32 = 1;
pub const QOS_SIGKILL: u32 = 9;
pub const QOS_SIGUSR1: u32 = 10;
pub const QOS_SIGUSR2: u32 = 12;
pub const QOS_SIGSTOP: u32 = 19;
pub const SIG_BLOCK: u32 = 0;
pub const SIG_UNBLOCK: u32 = 1;
pub const SIG_SETMASK: u32 = 2;

pub const QOS_TCP_STATE_CLOSED: u32 = 0;
pub const QOS_TCP_STATE_LISTEN: u32 = 1;
pub const QOS_TCP_STATE_SYN_SENT: u32 = 2;
pub const QOS_TCP_STATE_SYN_RECEIVED: u32 = 3;
pub const QOS_TCP_STATE_ESTABLISHED: u32 = 4;
pub const QOS_TCP_STATE_FIN_WAIT_1: u32 = 5;
pub const QOS_TCP_STATE_FIN_WAIT_2: u32 = 6;
pub const QOS_TCP_STATE_CLOSE_WAIT: u32 = 7;
pub const QOS_TCP_STATE_CLOSING: u32 = 8;
pub const QOS_TCP_STATE_LAST_ACK: u32 = 9;
pub const QOS_TCP_STATE_TIME_WAIT: u32 = 10;

pub const QOS_TCP_EVT_RX_SYN: u32 = 1;
pub const QOS_TCP_EVT_RX_SYN_ACK: u32 = 2;
pub const QOS_TCP_EVT_RX_ACK: u32 = 3;
pub const QOS_TCP_EVT_APP_CLOSE: u32 = 4;
pub const QOS_TCP_EVT_RX_FIN: u32 = 5;
pub const QOS_TCP_EVT_TIMEOUT: u32 = 6;

pub const SYSCALL_OP_CONST: u32 = 1;
pub const SYSCALL_OP_ADD: u32 = 2;
pub const SYSCALL_OP_QUERY: u32 = 3;
pub const SYSCALL_OP_KILL: u32 = 4;
pub const SYSCALL_OP_SIGPENDING: u32 = 5;
pub const SYSCALL_OP_SIGACTION: u32 = 6;
pub const SYSCALL_OP_SIGPROCMASK: u32 = 7;
pub const SYSCALL_OP_SIGALTSTACK: u32 = 8;
pub const SYSCALL_OP_SIGSUSPEND: u32 = 9;
pub const SYSCALL_OP_FORK: u32 = 10;
pub const SYSCALL_OP_EXEC: u32 = 11;
pub const SYSCALL_OP_SIGRETURN: u32 = 12;
pub const SYSCALL_OP_EXIT: u32 = 13;
pub const SYSCALL_OP_GETPID: u32 = 14;
pub const SYSCALL_OP_WAITPID: u32 = 15;
pub const SYSCALL_OP_STAT: u32 = 16;
pub const SYSCALL_OP_MKDIR: u32 = 17;
pub const SYSCALL_OP_UNLINK: u32 = 18;
pub const SYSCALL_OP_CHDIR: u32 = 19;
pub const SYSCALL_OP_GETCWD: u32 = 20;
pub const SYSCALL_OP_OPEN: u32 = 21;
pub const SYSCALL_OP_READ: u32 = 22;
pub const SYSCALL_OP_WRITE: u32 = 23;
pub const SYSCALL_OP_CLOSE: u32 = 24;
pub const SYSCALL_OP_DUP2: u32 = 25;
pub const SYSCALL_OP_MMAP: u32 = 26;
pub const SYSCALL_OP_MUNMAP: u32 = 27;
pub const SYSCALL_OP_YIELD: u32 = 28;
pub const SYSCALL_OP_SLEEP: u32 = 29;
pub const SYSCALL_OP_SOCKET: u32 = 30;
pub const SYSCALL_OP_BIND: u32 = 31;
pub const SYSCALL_OP_LISTEN: u32 = 32;
pub const SYSCALL_OP_ACCEPT: u32 = 33;
pub const SYSCALL_OP_CONNECT: u32 = 34;
pub const SYSCALL_OP_SEND: u32 = 35;
pub const SYSCALL_OP_RECV: u32 = 36;
pub const SYSCALL_OP_GETDENTS: u32 = 37;
pub const SYSCALL_OP_LSEEK: u32 = 38;
pub const SYSCALL_OP_PIPE: u32 = 39;
pub const SYSCALL_OP_DLOPEN: u32 = 40;
pub const SYSCALL_OP_DLCLOSE: u32 = 41;
pub const SYSCALL_OP_DLSYM: u32 = 42;
pub const SYSCALL_OP_MODLOAD: u32 = 43;
pub const SYSCALL_OP_MODUNLOAD: u32 = 44;
pub const SYSCALL_OP_MODRELOAD: u32 = 45;

pub const SYSCALL_QUERY_INIT_STATE: u32 = 1;
pub const SYSCALL_QUERY_PMM_TOTAL: u32 = 2;
pub const SYSCALL_QUERY_PMM_FREE: u32 = 3;
pub const SYSCALL_QUERY_SCHED_COUNT: u32 = 4;
pub const SYSCALL_QUERY_VFS_COUNT: u32 = 5;
pub const SYSCALL_QUERY_NET_QUEUE_LEN: u32 = 6;
pub const SYSCALL_QUERY_DRIVERS_COUNT: u32 = 7;
pub const SYSCALL_QUERY_SYSCALL_COUNT: u32 = 8;
pub const SYSCALL_QUERY_PROC_COUNT: u32 = 9;

pub const SYSCALL_NR_FORK: u32 = 0;
pub const SYSCALL_NR_EXEC: u32 = 1;
pub const SYSCALL_NR_EXIT: u32 = 2;
pub const SYSCALL_NR_GETPID: u32 = 3;
pub const SYSCALL_NR_WAITPID: u32 = 4;
pub const SYSCALL_NR_OPEN: u32 = 5;
pub const SYSCALL_NR_READ: u32 = 6;
pub const SYSCALL_NR_WRITE: u32 = 7;
pub const SYSCALL_NR_CLOSE: u32 = 8;
pub const SYSCALL_NR_DUP2: u32 = 9;
pub const SYSCALL_NR_MMAP: u32 = 10;
pub const SYSCALL_NR_MUNMAP: u32 = 11;
pub const SYSCALL_NR_YIELD: u32 = 12;
pub const SYSCALL_NR_SLEEP: u32 = 13;
pub const SYSCALL_NR_SOCKET: u32 = 14;
pub const SYSCALL_NR_BIND: u32 = 15;
pub const SYSCALL_NR_LISTEN: u32 = 16;
pub const SYSCALL_NR_ACCEPT: u32 = 17;
pub const SYSCALL_NR_CONNECT: u32 = 18;
pub const SYSCALL_NR_SEND: u32 = 19;
pub const SYSCALL_NR_RECV: u32 = 20;
pub const SYSCALL_NR_STAT: u32 = 21;
pub const SYSCALL_NR_MKDIR: u32 = 22;
pub const SYSCALL_NR_UNLINK: u32 = 23;
pub const SYSCALL_NR_GETDENTS: u32 = 24;
pub const SYSCALL_NR_LSEEK: u32 = 25;
pub const SYSCALL_NR_PIPE: u32 = 26;
pub const SYSCALL_NR_CHDIR: u32 = 27;
pub const SYSCALL_NR_GETCWD: u32 = 28;

pub const SYSCALL_NR_KILL: u32 = 29;
pub const SYSCALL_NR_SIGACTION: u32 = 30;
pub const SYSCALL_NR_SIGPROCMASK: u32 = 31;
pub const SYSCALL_NR_SIGRETURN: u32 = 32;
pub const SYSCALL_NR_SIGPENDING: u32 = 33;
pub const SYSCALL_NR_SIGSUSPEND: u32 = 34;
pub const SYSCALL_NR_SIGALTSTACK: u32 = 35;
pub const SYSCALL_NR_DLOPEN: u32 = 36;
pub const SYSCALL_NR_DLCLOSE: u32 = 37;
pub const SYSCALL_NR_DLSYM: u32 = 38;
pub const SYSCALL_NR_MODLOAD: u32 = 39;
pub const SYSCALL_NR_MODUNLOAD: u32 = 40;
pub const SYSCALL_NR_MODRELOAD: u32 = 41;

pub const SYSCALL_NR_QUERY_INIT_STATE: u32 = 64;
pub const SYSCALL_NR_QUERY_PMM_TOTAL: u32 = 65;
pub const SYSCALL_NR_QUERY_PMM_FREE: u32 = 66;
pub const SYSCALL_NR_QUERY_SCHED_COUNT: u32 = 67;
pub const SYSCALL_NR_QUERY_VFS_COUNT: u32 = 68;
pub const SYSCALL_NR_QUERY_NET_QUEUE_LEN: u32 = 69;
pub const SYSCALL_NR_QUERY_DRIVERS_COUNT: u32 = 70;
pub const SYSCALL_NR_QUERY_SYSCALL_COUNT: u32 = 71;
pub const SYSCALL_NR_QUERY_PROC_COUNT: u32 = 72;

const FD_KIND_NONE: u8 = 0;
const FD_KIND_VFS: u8 = 1;
const FD_KIND_SOCKET: u8 = 2;
const FD_KIND_PIPE_R: u8 = 3;
const FD_KIND_PIPE_W: u8 = 4;
const FD_KIND_PROC: u8 = 5;
const SOCK_STREAM: u8 = 1;
const SOCK_DGRAM: u8 = 2;
const SOCK_RAW: u8 = 3;
const PROC_FD_NONE: u8 = 0;
const PROC_FD_MEMINFO: u8 = 1;
const PROC_FD_STATUS: u8 = 2;
const PROC_FD_KERNEL_STATUS: u8 = 3;
const PROC_FD_NET_DEV: u8 = 4;
const PROC_FD_SYSCALLS: u8 = 5;
const PROC_FD_UPTIME: u8 = 6;
const PROC_FD_RUNTIME_MAP: u8 = 7;
const QOS_SOCK_PORT_MIN: u16 = 1024;
const QOS_SOCK_PORT_MAX: u16 = 65535;
const QOS_SOCK_PORT_COUNT: usize = QOS_SOCK_PORT_MAX as usize - QOS_SOCK_PORT_MIN as usize + 1;
const QOS_SOCK_PORT_WORDS: usize = (QOS_SOCK_PORT_COUNT + 31) / 32;
const QOS_SOCK_PENDING_MAX: usize = 16;
const QOS_SHLIB_MAX: usize = 64;
const QOS_MODULE_MAX: usize = 32;
const ELF_CLASS_64: u8 = 2;
const ELF_DATA_LSB: u8 = 1;
const ELF_ET_EXEC: u16 = 2;
const ELF_ET_DYN: u16 = 3;
const ELF_EM_X86_64: u16 = 0x3E;
const ELF_EM_AARCH64: u16 = 0xB7;
const ELF_PT_LOAD: u32 = 1;
const ELF_PT_INTERP: u32 = 3;
type SoftirqHandler = fn(usize);
type TimerCallback = fn(usize);
type KthreadFn = fn(usize);
type NapiPoll = fn(usize, u32) -> u32;
type WorkqueueFn = fn(usize);

#[derive(Clone, Copy)]
struct SchedState {
    pids: [u32; QOS_SCHED_MAX_TASKS],
    count: u32,
    cursor: u32,
    current_pid: u32,
}

impl SchedState {
    const fn new() -> Self {
        Self {
            pids: [0; QOS_SCHED_MAX_TASKS],
            count: 0,
            cursor: 0,
            current_pid: 0,
        }
    }
}

struct SchedCell(UnsafeCell<SchedState>);

unsafe impl Sync for SchedCell {}

static SCHED: SchedCell = SchedCell(UnsafeCell::new(SchedState::new()));

#[derive(Clone, Copy)]
struct VfsState {
    paths: [[u8; QOS_VFS_PATH_MAX]; QOS_VFS_MAX_NODES],
    lens: [u8; QOS_VFS_MAX_NODES],
    used: [u8; QOS_VFS_MAX_NODES],
    count: u32,
}

impl VfsState {
    const fn new() -> Self {
        Self {
            paths: [[0; QOS_VFS_PATH_MAX]; QOS_VFS_MAX_NODES],
            lens: [0; QOS_VFS_MAX_NODES],
            used: [0; QOS_VFS_MAX_NODES],
            count: 0,
        }
    }
}

struct VfsCell(UnsafeCell<VfsState>);

unsafe impl Sync for VfsCell {}

static VFS: VfsCell = VfsCell(UnsafeCell::new(VfsState::new()));

#[derive(Clone, Copy)]
struct NetState {
    packets: [[u8; QOS_NET_MAX_PACKET]; QOS_NET_MAX_QUEUE],
    lens: [u16; QOS_NET_MAX_QUEUE],
    head: u32,
    tail: u32,
    count: u32,
    arp_used: [u8; QOS_NET_ARP_MAX],
    arp_ip: [u32; QOS_NET_ARP_MAX],
    arp_mac: [[u8; 6]; QOS_NET_ARP_MAX],
    arp_ttl_secs: [u32; QOS_NET_ARP_MAX],
    udp_port_used: [u8; QOS_NET_UDP_PORT_MAX],
    udp_ports: [u16; QOS_NET_UDP_PORT_MAX],
    udp_used: [u8; QOS_NET_UDP_MAX_DATAGRAMS],
    udp_dst_port: [u16; QOS_NET_UDP_MAX_DATAGRAMS],
    udp_src_port: [u16; QOS_NET_UDP_MAX_DATAGRAMS],
    udp_src_ip: [u32; QOS_NET_UDP_MAX_DATAGRAMS],
    udp_len: [u16; QOS_NET_UDP_MAX_DATAGRAMS],
    udp_data: [[u8; QOS_NET_MAX_PACKET]; QOS_NET_UDP_MAX_DATAGRAMS],
    udp_count: u32,
    tcp_listener_used: [u8; QOS_NET_TCP_MAX_LISTENERS],
    tcp_listeners: [u16; QOS_NET_TCP_MAX_LISTENERS],
    tcp_conn_used: [u8; QOS_NET_TCP_MAX_CONNS],
    tcp_conn_state: [u8; QOS_NET_TCP_MAX_CONNS],
    tcp_local_port: [u16; QOS_NET_TCP_MAX_CONNS],
    tcp_remote_port: [u16; QOS_NET_TCP_MAX_CONNS],
    tcp_remote_ip: [u32; QOS_NET_TCP_MAX_CONNS],
    tcp_rto_ms: [u32; QOS_NET_TCP_MAX_CONNS],
    tcp_retries: [u8; QOS_NET_TCP_MAX_CONNS],
}

impl NetState {
    const fn new() -> Self {
        Self {
            packets: [[0; QOS_NET_MAX_PACKET]; QOS_NET_MAX_QUEUE],
            lens: [0; QOS_NET_MAX_QUEUE],
            head: 0,
            tail: 0,
            count: 0,
            arp_used: [0; QOS_NET_ARP_MAX],
            arp_ip: [0; QOS_NET_ARP_MAX],
            arp_mac: [[0; 6]; QOS_NET_ARP_MAX],
            arp_ttl_secs: [0; QOS_NET_ARP_MAX],
            udp_port_used: [0; QOS_NET_UDP_PORT_MAX],
            udp_ports: [0; QOS_NET_UDP_PORT_MAX],
            udp_used: [0; QOS_NET_UDP_MAX_DATAGRAMS],
            udp_dst_port: [0; QOS_NET_UDP_MAX_DATAGRAMS],
            udp_src_port: [0; QOS_NET_UDP_MAX_DATAGRAMS],
            udp_src_ip: [0; QOS_NET_UDP_MAX_DATAGRAMS],
            udp_len: [0; QOS_NET_UDP_MAX_DATAGRAMS],
            udp_data: [[0; QOS_NET_MAX_PACKET]; QOS_NET_UDP_MAX_DATAGRAMS],
            udp_count: 0,
            tcp_listener_used: [0; QOS_NET_TCP_MAX_LISTENERS],
            tcp_listeners: [0; QOS_NET_TCP_MAX_LISTENERS],
            tcp_conn_used: [0; QOS_NET_TCP_MAX_CONNS],
            tcp_conn_state: [0; QOS_NET_TCP_MAX_CONNS],
            tcp_local_port: [0; QOS_NET_TCP_MAX_CONNS],
            tcp_remote_port: [0; QOS_NET_TCP_MAX_CONNS],
            tcp_remote_ip: [0; QOS_NET_TCP_MAX_CONNS],
            tcp_rto_ms: [0; QOS_NET_TCP_MAX_CONNS],
            tcp_retries: [0; QOS_NET_TCP_MAX_CONNS],
        }
    }
}

struct NetCell(UnsafeCell<NetState>);

unsafe impl Sync for NetCell {}

static NET: NetCell = NetCell(UnsafeCell::new(NetState::new()));

#[derive(Clone, Copy)]
struct DriversState {
    names: [[u8; QOS_DRIVER_NAME_MAX]; QOS_DRIVERS_MAX],
    lens: [u8; QOS_DRIVERS_MAX],
    used: [u8; QOS_DRIVERS_MAX],
    nic_present: [u8; QOS_DRIVERS_MAX],
    nic_mmio_base: [u64; QOS_DRIVERS_MAX],
    nic_irq: [u32; QOS_DRIVERS_MAX],
    nic_tx_desc_count: [u16; QOS_DRIVERS_MAX],
    nic_rx_desc_count: [u16; QOS_DRIVERS_MAX],
    nic_tx_head: [u16; QOS_DRIVERS_MAX],
    nic_tx_tail: [u16; QOS_DRIVERS_MAX],
    nic_rx_head: [u16; QOS_DRIVERS_MAX],
    nic_rx_tail: [u16; QOS_DRIVERS_MAX],
    count: u32,
}

impl DriversState {
    const fn new() -> Self {
        Self {
            names: [[0; QOS_DRIVER_NAME_MAX]; QOS_DRIVERS_MAX],
            lens: [0; QOS_DRIVERS_MAX],
            used: [0; QOS_DRIVERS_MAX],
            nic_present: [0; QOS_DRIVERS_MAX],
            nic_mmio_base: [0; QOS_DRIVERS_MAX],
            nic_irq: [0; QOS_DRIVERS_MAX],
            nic_tx_desc_count: [0; QOS_DRIVERS_MAX],
            nic_rx_desc_count: [0; QOS_DRIVERS_MAX],
            nic_tx_head: [0; QOS_DRIVERS_MAX],
            nic_tx_tail: [0; QOS_DRIVERS_MAX],
            nic_rx_head: [0; QOS_DRIVERS_MAX],
            nic_rx_tail: [0; QOS_DRIVERS_MAX],
            count: 0,
        }
    }
}

struct DriversCell(UnsafeCell<DriversState>);

unsafe impl Sync for DriversCell {}

static DRIVERS: DriversCell = DriversCell(UnsafeCell::new(DriversState::new()));

#[derive(Clone, Copy)]
struct SyscallState {
    used: [u8; QOS_SYSCALL_MAX],
    ops: [u8; QOS_SYSCALL_MAX],
    values: [i64; QOS_SYSCALL_MAX],
    count: u32,
    fd_used: [u8; QOS_FD_MAX],
    fd_kind: [u8; QOS_FD_MAX],
    fd_sock_bound: [u8; QOS_FD_MAX],
    fd_sock_listening: [u8; QOS_FD_MAX],
    fd_sock_connected: [u8; QOS_FD_MAX],
    fd_sock_pending: [u8; QOS_FD_MAX],
    fd_sock_backlog: [u8; QOS_FD_MAX],
    fd_sock_type: [u8; QOS_FD_MAX],
    fd_sock_lport: [u16; QOS_FD_MAX],
    fd_sock_rport: [u16; QOS_FD_MAX],
    fd_sock_rip: [u32; QOS_FD_MAX],
    fd_sock_conn_id: [i16; QOS_FD_MAX],
    fd_sock_stream_len: [u16; QOS_FD_MAX],
    fd_sock_stream_buf: [[u8; QOS_NET_MAX_PACKET]; QOS_FD_MAX],
    fd_sock_raw_len: [u16; QOS_FD_MAX],
    fd_sock_raw_src_ip: [u32; QOS_FD_MAX],
    fd_sock_raw_buf: [[u8; QOS_NET_MAX_PACKET]; QOS_FD_MAX],
    fd_sock_pending_rport: [u16; QOS_FD_MAX],
    fd_sock_pending_rip: [u32; QOS_FD_MAX],
    fd_sock_pending_conn_id: [i16; QOS_FD_MAX],
    fd_sock_pending_head: [u8; QOS_FD_MAX],
    fd_sock_pending_tail: [u8; QOS_FD_MAX],
    fd_sock_pending_q_rport: [[u16; QOS_SOCK_PENDING_MAX]; QOS_FD_MAX],
    fd_sock_pending_q_rip: [[u32; QOS_SOCK_PENDING_MAX]; QOS_FD_MAX],
    fd_sock_pending_q_conn_id: [[i16; QOS_SOCK_PENDING_MAX]; QOS_FD_MAX],
    fd_pipe_id: [u16; QOS_FD_MAX],
    fd_offset: [u64; QOS_FD_MAX],
    fd_file_id: [u16; QOS_FD_MAX],
    fd_proc_kind: [u8; QOS_FD_MAX],
    fd_proc_pid: [u32; QOS_FD_MAX],
    file_used: [u8; QOS_VFS_MAX_NODES],
    file_paths: [[u8; QOS_VFS_PATH_MAX]; QOS_VFS_MAX_NODES],
    file_path_len: [u8; QOS_VFS_MAX_NODES],
    file_len: [u16; QOS_VFS_MAX_NODES],
    file_data: [[u8; QOS_PIPE_CAP]; QOS_VFS_MAX_NODES],
    pipe_used: [u8; QOS_PIPE_MAX],
    pipe_len: [u16; QOS_PIPE_MAX],
    pipe_buf: [[u8; QOS_PIPE_CAP]; QOS_PIPE_MAX],
    sock_port_stream: [u32; QOS_SOCK_PORT_WORDS],
    sock_port_dgram: [u32; QOS_SOCK_PORT_WORDS],
    shlib_used: [u8; QOS_SHLIB_MAX],
    shlib_handle: [u32; QOS_SHLIB_MAX],
    shlib_refcount: [u32; QOS_SHLIB_MAX],
    shlib_file_id: [u16; QOS_SHLIB_MAX],
    shlib_paths: [[u8; QOS_VFS_PATH_MAX]; QOS_SHLIB_MAX],
    shlib_path_len: [u8; QOS_SHLIB_MAX],
    shlib_next_handle: u32,
    module_used: [u8; QOS_MODULE_MAX],
    module_id: [u32; QOS_MODULE_MAX],
    module_generation: [u32; QOS_MODULE_MAX],
    module_shlib_handle: [u32; QOS_MODULE_MAX],
    module_paths: [[u8; QOS_VFS_PATH_MAX]; QOS_MODULE_MAX],
    module_path_len: [u8; QOS_MODULE_MAX],
    module_next_id: u32,
}

impl SyscallState {
    const fn new() -> Self {
        Self {
            used: [0; QOS_SYSCALL_MAX],
            ops: [0; QOS_SYSCALL_MAX],
            values: [0; QOS_SYSCALL_MAX],
            count: 0,
            fd_used: [0; QOS_FD_MAX],
            fd_kind: [FD_KIND_NONE; QOS_FD_MAX],
            fd_sock_bound: [0; QOS_FD_MAX],
            fd_sock_listening: [0; QOS_FD_MAX],
            fd_sock_connected: [0; QOS_FD_MAX],
            fd_sock_pending: [0; QOS_FD_MAX],
            fd_sock_backlog: [0; QOS_FD_MAX],
            fd_sock_type: [0; QOS_FD_MAX],
            fd_sock_lport: [0; QOS_FD_MAX],
            fd_sock_rport: [0; QOS_FD_MAX],
            fd_sock_rip: [0; QOS_FD_MAX],
            fd_sock_conn_id: [-1; QOS_FD_MAX],
            fd_sock_stream_len: [0; QOS_FD_MAX],
            fd_sock_stream_buf: [[0; QOS_NET_MAX_PACKET]; QOS_FD_MAX],
            fd_sock_raw_len: [0; QOS_FD_MAX],
            fd_sock_raw_src_ip: [0; QOS_FD_MAX],
            fd_sock_raw_buf: [[0; QOS_NET_MAX_PACKET]; QOS_FD_MAX],
            fd_sock_pending_rport: [0; QOS_FD_MAX],
            fd_sock_pending_rip: [0; QOS_FD_MAX],
            fd_sock_pending_conn_id: [-1; QOS_FD_MAX],
            fd_sock_pending_head: [0; QOS_FD_MAX],
            fd_sock_pending_tail: [0; QOS_FD_MAX],
            fd_sock_pending_q_rport: [[0; QOS_SOCK_PENDING_MAX]; QOS_FD_MAX],
            fd_sock_pending_q_rip: [[0; QOS_SOCK_PENDING_MAX]; QOS_FD_MAX],
            fd_sock_pending_q_conn_id: [[-1; QOS_SOCK_PENDING_MAX]; QOS_FD_MAX],
            fd_pipe_id: [0; QOS_FD_MAX],
            fd_offset: [0; QOS_FD_MAX],
            fd_file_id: [u16::MAX; QOS_FD_MAX],
            fd_proc_kind: [PROC_FD_NONE; QOS_FD_MAX],
            fd_proc_pid: [0; QOS_FD_MAX],
            file_used: [0; QOS_VFS_MAX_NODES],
            file_paths: [[0; QOS_VFS_PATH_MAX]; QOS_VFS_MAX_NODES],
            file_path_len: [0; QOS_VFS_MAX_NODES],
            file_len: [0; QOS_VFS_MAX_NODES],
            file_data: [[0; QOS_PIPE_CAP]; QOS_VFS_MAX_NODES],
            pipe_used: [0; QOS_PIPE_MAX],
            pipe_len: [0; QOS_PIPE_MAX],
            pipe_buf: [[0; QOS_PIPE_CAP]; QOS_PIPE_MAX],
            sock_port_stream: [0; QOS_SOCK_PORT_WORDS],
            sock_port_dgram: [0; QOS_SOCK_PORT_WORDS],
            shlib_used: [0; QOS_SHLIB_MAX],
            shlib_handle: [0; QOS_SHLIB_MAX],
            shlib_refcount: [0; QOS_SHLIB_MAX],
            shlib_file_id: [u16::MAX; QOS_SHLIB_MAX],
            shlib_paths: [[0; QOS_VFS_PATH_MAX]; QOS_SHLIB_MAX],
            shlib_path_len: [0; QOS_SHLIB_MAX],
            shlib_next_handle: 1,
            module_used: [0; QOS_MODULE_MAX],
            module_id: [0; QOS_MODULE_MAX],
            module_generation: [0; QOS_MODULE_MAX],
            module_shlib_handle: [0; QOS_MODULE_MAX],
            module_paths: [[0; QOS_VFS_PATH_MAX]; QOS_MODULE_MAX],
            module_path_len: [0; QOS_MODULE_MAX],
            module_next_id: 1,
        }
    }
}

struct SyscallCell(UnsafeCell<SyscallState>);

unsafe impl Sync for SyscallCell {}

static SYSCALLS: SyscallCell = SyscallCell(UnsafeCell::new(SyscallState::new()));

#[derive(Clone, Copy)]
struct ProcState {
    pids: [u32; QOS_PROC_MAX],
    ppids: [u32; QOS_PROC_MAX],
    used: [u8; QOS_PROC_MAX],
    exited: [u8; QOS_PROC_MAX],
    exit_code: [i32; QOS_PROC_MAX],
    cwd: [[u8; QOS_VFS_PATH_MAX]; QOS_PROC_MAX],
    cwd_len: [u8; QOS_PROC_MAX],
    names: [[u8; QOS_PROC_NAME_MAX]; QOS_PROC_MAX],
    name_len: [u8; QOS_PROC_MAX],
    sig_handlers: [[u32; QOS_SIGNAL_MAX]; QOS_PROC_MAX],
    sig_pending: [u32; QOS_PROC_MAX],
    sig_blocked: [u32; QOS_PROC_MAX],
    sig_altstack_sp: [u64; QOS_PROC_MAX],
    sig_altstack_size: [u64; QOS_PROC_MAX],
    sig_altstack_flags: [u32; QOS_PROC_MAX],
    exec_entry: [u64; QOS_PROC_MAX],
    exec_phoff: [u64; QOS_PROC_MAX],
    exec_phentsize: [u16; QOS_PROC_MAX],
    exec_phnum: [u16; QOS_PROC_MAX],
    exec_load_count: [u16; QOS_PROC_MAX],
    exec_has_interp: [u8; QOS_PROC_MAX],
    exec_interp_off: [u64; QOS_PROC_MAX],
    exec_interp_len: [u64; QOS_PROC_MAX],
    count: u32,
}

impl ProcState {
    const fn new() -> Self {
        Self {
            pids: [0; QOS_PROC_MAX],
            ppids: [0; QOS_PROC_MAX],
            used: [0; QOS_PROC_MAX],
            exited: [0; QOS_PROC_MAX],
            exit_code: [0; QOS_PROC_MAX],
            cwd: [[0; QOS_VFS_PATH_MAX]; QOS_PROC_MAX],
            cwd_len: [0; QOS_PROC_MAX],
            names: [[0; QOS_PROC_NAME_MAX]; QOS_PROC_MAX],
            name_len: [0; QOS_PROC_MAX],
            sig_handlers: [[0; QOS_SIGNAL_MAX]; QOS_PROC_MAX],
            sig_pending: [0; QOS_PROC_MAX],
            sig_blocked: [0; QOS_PROC_MAX],
            sig_altstack_sp: [0; QOS_PROC_MAX],
            sig_altstack_size: [0; QOS_PROC_MAX],
            sig_altstack_flags: [0; QOS_PROC_MAX],
            exec_entry: [0; QOS_PROC_MAX],
            exec_phoff: [0; QOS_PROC_MAX],
            exec_phentsize: [0; QOS_PROC_MAX],
            exec_phnum: [0; QOS_PROC_MAX],
            exec_load_count: [0; QOS_PROC_MAX],
            exec_has_interp: [0; QOS_PROC_MAX],
            exec_interp_off: [0; QOS_PROC_MAX],
            exec_interp_len: [0; QOS_PROC_MAX],
            count: 0,
        }
    }
}

struct ProcCell(UnsafeCell<ProcState>);

unsafe impl Sync for ProcCell {}

static PROCS: ProcCell = ProcCell(UnsafeCell::new(ProcState::new()));

#[derive(Clone, Copy)]
struct SoftirqState {
    handlers: [Option<SoftirqHandler>; QOS_SOFTIRQ_MAX],
    handler_ctx: [usize; QOS_SOFTIRQ_MAX],
    pending_mask: u32,
}

impl SoftirqState {
    const fn new() -> Self {
        Self {
            handlers: [None; QOS_SOFTIRQ_MAX],
            handler_ctx: [0; QOS_SOFTIRQ_MAX],
            pending_mask: 0,
        }
    }
}

struct SoftirqCell(UnsafeCell<SoftirqState>);

unsafe impl Sync for SoftirqCell {}

static SOFTIRQ: SoftirqCell = SoftirqCell(UnsafeCell::new(SoftirqState::new()));

#[derive(Clone, Copy)]
struct TimerEntry {
    used: u8,
    periodic: u8,
    pending: u8,
    expires: u64,
    interval: u32,
    callback: Option<TimerCallback>,
    ctx: usize,
}

impl TimerEntry {
    const fn new() -> Self {
        Self {
            used: 0,
            periodic: 0,
            pending: 0,
            expires: 0,
            interval: 0,
            callback: None,
            ctx: 0,
        }
    }
}

#[derive(Clone, Copy)]
struct TimerState {
    entries: [TimerEntry; QOS_TIMER_MAX],
    jiffies: u64,
}

impl TimerState {
    const fn new() -> Self {
        Self {
            entries: [TimerEntry::new(); QOS_TIMER_MAX],
            jiffies: 0,
        }
    }
}

struct TimerCell(UnsafeCell<TimerState>);

unsafe impl Sync for TimerCell {}

static TIMERS: TimerCell = TimerCell(UnsafeCell::new(TimerState::new()));

#[derive(Clone, Copy)]
struct KthreadEntry {
    used: u8,
    runnable: u8,
    tid: u32,
    entry: Option<KthreadFn>,
    arg: usize,
    run_count: u32,
}

impl KthreadEntry {
    const fn new() -> Self {
        Self {
            used: 0,
            runnable: 0,
            tid: 0,
            entry: None,
            arg: 0,
            run_count: 0,
        }
    }
}

#[derive(Clone, Copy)]
struct KthreadState {
    entries: [KthreadEntry; QOS_KTHREAD_MAX],
    names: [[u8; QOS_KTHREAD_NAME_MAX]; QOS_KTHREAD_MAX],
    name_len: [u8; QOS_KTHREAD_MAX],
    next_tid: u32,
    cursor: u32,
    current_tid: u32,
}

impl KthreadState {
    const fn new() -> Self {
        Self {
            entries: [KthreadEntry::new(); QOS_KTHREAD_MAX],
            names: [[0; QOS_KTHREAD_NAME_MAX]; QOS_KTHREAD_MAX],
            name_len: [0; QOS_KTHREAD_MAX],
            next_tid: 1,
            cursor: 0,
            current_tid: 0,
        }
    }
}

struct KthreadCell(UnsafeCell<KthreadState>);

unsafe impl Sync for KthreadCell {}

static KTHREADS: KthreadCell = KthreadCell(UnsafeCell::new(KthreadState::new()));

#[derive(Clone, Copy)]
struct NapiEntry {
    used: u8,
    scheduled: u8,
    id: u32,
    weight: u32,
    poll: Option<NapiPoll>,
    ctx: usize,
    run_count: u32,
}

impl NapiEntry {
    const fn new() -> Self {
        Self {
            used: 0,
            scheduled: 0,
            id: 0,
            weight: 0,
            poll: None,
            ctx: 0,
            run_count: 0,
        }
    }
}

#[derive(Clone, Copy)]
struct NapiState {
    entries: [NapiEntry; QOS_NAPI_MAX],
    next_id: u32,
}

impl NapiState {
    const fn new() -> Self {
        Self {
            entries: [NapiEntry::new(); QOS_NAPI_MAX],
            next_id: 1,
        }
    }
}

struct NapiCell(UnsafeCell<NapiState>);

unsafe impl Sync for NapiCell {}

static NAPI: NapiCell = NapiCell(UnsafeCell::new(NapiState::new()));

#[derive(Clone, Copy)]
struct WorkqueueEntry {
    used: u8,
    pending: u8,
    id: u32,
    work: Option<WorkqueueFn>,
    arg: usize,
}

impl WorkqueueEntry {
    const fn new() -> Self {
        Self {
            used: 0,
            pending: 0,
            id: 0,
            work: None,
            arg: 0,
        }
    }
}

#[derive(Clone, Copy)]
struct WorkqueueState {
    entries: [WorkqueueEntry; QOS_WORKQUEUE_MAX],
    queue: [u16; QOS_WORKQUEUE_MAX],
    qhead: u16,
    qtail: u16,
    qcount: u16,
    next_id: u32,
    completed: u32,
}

impl WorkqueueState {
    const fn new() -> Self {
        Self {
            entries: [WorkqueueEntry::new(); QOS_WORKQUEUE_MAX],
            queue: [0; QOS_WORKQUEUE_MAX],
            qhead: 0,
            qtail: 0,
            qcount: 0,
            next_id: 1,
            completed: 0,
        }
    }
}

struct WorkqueueCell(UnsafeCell<WorkqueueState>);

unsafe impl Sync for WorkqueueCell {}

static WORKQUEUE: WorkqueueCell = WorkqueueCell(UnsafeCell::new(WorkqueueState::new()));

fn pmm_state_mut() -> &'static mut PmmState {
    unsafe { &mut *PMM.0.get() }
}

fn pmm_state() -> &'static PmmState {
    unsafe { &*PMM.0.get() }
}

fn vmm_state_mut() -> &'static mut VmmState {
    unsafe { &mut *VMM.0.get() }
}

fn vmm_state() -> &'static VmmState {
    unsafe { &*VMM.0.get() }
}

fn sched_state_mut() -> &'static mut SchedState {
    unsafe { &mut *SCHED.0.get() }
}

fn sched_state() -> &'static SchedState {
    unsafe { &*SCHED.0.get() }
}

fn vfs_state_mut() -> &'static mut VfsState {
    unsafe { &mut *VFS.0.get() }
}

fn vfs_state() -> &'static VfsState {
    unsafe { &*VFS.0.get() }
}

fn net_state_mut() -> &'static mut NetState {
    unsafe { &mut *NET.0.get() }
}

fn net_state() -> &'static NetState {
    unsafe { &*NET.0.get() }
}

fn drivers_state_mut() -> &'static mut DriversState {
    unsafe { &mut *DRIVERS.0.get() }
}

fn drivers_state() -> &'static DriversState {
    unsafe { &*DRIVERS.0.get() }
}

fn syscall_state_mut() -> &'static mut SyscallState {
    unsafe { &mut *SYSCALLS.0.get() }
}

fn syscall_state() -> &'static SyscallState {
    unsafe { &*SYSCALLS.0.get() }
}

fn proc_state_mut() -> &'static mut ProcState {
    unsafe { &mut *PROCS.0.get() }
}

fn proc_state() -> &'static ProcState {
    unsafe { &*PROCS.0.get() }
}

fn softirq_state_mut() -> &'static mut SoftirqState {
    unsafe { &mut *SOFTIRQ.0.get() }
}

fn softirq_state() -> &'static SoftirqState {
    unsafe { &*SOFTIRQ.0.get() }
}

fn timer_state_mut() -> &'static mut TimerState {
    unsafe { &mut *TIMERS.0.get() }
}

fn timer_state() -> &'static TimerState {
    unsafe { &*TIMERS.0.get() }
}

fn kthread_state_mut() -> &'static mut KthreadState {
    unsafe { &mut *KTHREADS.0.get() }
}

fn kthread_state() -> &'static KthreadState {
    unsafe { &*KTHREADS.0.get() }
}

fn napi_state_mut() -> &'static mut NapiState {
    unsafe { &mut *NAPI.0.get() }
}

fn napi_state() -> &'static NapiState {
    unsafe { &*NAPI.0.get() }
}

fn workqueue_state_mut() -> &'static mut WorkqueueState {
    unsafe { &mut *WORKQUEUE.0.get() }
}

fn workqueue_state() -> &'static WorkqueueState {
    unsafe { &*WORKQUEUE.0.get() }
}

fn zero_struct<T>(value: &mut T) {
    unsafe {
        core::ptr::write_bytes(value as *mut T as *mut u8, 0, core::mem::size_of::<T>());
    }
}

fn syscall_state_reset_in_place(state: &mut SyscallState) {
    zero_struct(state);

    let mut i = 0usize;
    while i < QOS_FD_MAX {
        state.fd_sock_conn_id[i] = -1;
        state.fd_sock_pending_conn_id[i] = -1;
        state.fd_file_id[i] = u16::MAX;
        let mut j = 0usize;
        while j < QOS_SOCK_PENDING_MAX {
            state.fd_sock_pending_q_conn_id[i][j] = -1;
            j += 1;
        }
        i += 1;
    }

    i = 0;
    while i < QOS_SHLIB_MAX {
        state.shlib_file_id[i] = u16::MAX;
        i += 1;
    }

    state.shlib_next_handle = 1;
    state.module_next_id = 1;
}

fn mm_lock() {
    while MM_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn mm_unlock() {
    MM_LOCK.store(false, Ordering::Release);
}

fn sched_lock() {
    while SCHED_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn sched_unlock() {
    SCHED_LOCK.store(false, Ordering::Release);
}

fn vfs_lock() {
    while VFS_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn vfs_unlock() {
    VFS_LOCK.store(false, Ordering::Release);
}

fn net_lock() {
    while NET_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn net_unlock() {
    NET_LOCK.store(false, Ordering::Release);
}

fn drivers_lock() {
    while DRIVERS_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn drivers_unlock() {
    DRIVERS_LOCK.store(false, Ordering::Release);
}

fn syscall_lock() {
    while SYSCALL_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn syscall_unlock() {
    SYSCALL_LOCK.store(false, Ordering::Release);
}

fn proc_lock() {
    while PROC_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn proc_unlock() {
    PROC_LOCK.store(false, Ordering::Release);
}

fn softirq_lock() {
    while SOFTIRQ_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn softirq_unlock() {
    SOFTIRQ_LOCK.store(false, Ordering::Release);
}

fn timer_lock() {
    while TIMER_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn timer_unlock() {
    TIMER_LOCK.store(false, Ordering::Release);
}

fn kthread_lock() {
    while KTHREAD_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn kthread_unlock() {
    KTHREAD_LOCK.store(false, Ordering::Release);
}

fn napi_lock() {
    while NAPI_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn napi_unlock() {
    NAPI_LOCK.store(false, Ordering::Release);
}

fn workqueue_lock() {
    while WORKQUEUE_LOCK
        .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        spin_loop();
    }
}

fn workqueue_unlock() {
    WORKQUEUE_LOCK.store(false, Ordering::Release);
}

fn align_up(value: u64) -> u64 {
    (value + (QOS_PAGE_SIZE - 1)) & !(QOS_PAGE_SIZE - 1)
}

fn align_down(value: u64) -> u64 {
    value & !(QOS_PAGE_SIZE - 1)
}

fn is_page_aligned(value: u64) -> bool {
    (value & (QOS_PAGE_SIZE - 1)) == 0
}

fn mark(bit: u32) {
    INIT_STATE.fetch_or(bit, Ordering::SeqCst);
}

fn mm_init() {
    mark(QOS_INIT_MM);
}

fn drivers_init() {
    drivers_reset();
    mark(QOS_INIT_DRIVERS);
}

fn vfs_init() {
    vfs_reset();
    mark(QOS_INIT_VFS);
}

fn net_init() {
    net_reset();
    mark(QOS_INIT_NET);
}

pub fn softirq_init() {
    softirq_reset();
}

pub fn timer_init() {
    timer_reset();
    let _ = softirq_register(QOS_SOFTIRQ_TIMER, timer_softirq_handler, 0);
}

pub fn napi_init() {
    napi_reset();
    let _ = softirq_register(QOS_SOFTIRQ_NET_RX, napi_softirq_handler, 0);
}

pub fn kthread_init() {
    kthread_reset();
}

pub fn workqueue_init() {
    workqueue_reset();
    let _ = softirq_register(QOS_SOFTIRQ_WORKQUEUE, workqueue_softirq_handler, 0);
}

fn sched_init() {
    sched_reset();
    mark(QOS_INIT_SCHED);
}

fn syscall_init() {
    syscall_reset();
    syscall_register_default_signal_ops();
    syscall_register_default_queries();
    mark(QOS_INIT_SYSCALL);
}

fn syscall_query_value(query: u32, syscall_count: u32) -> i64 {
    match query {
        SYSCALL_QUERY_INIT_STATE => init_state() as i64,
        SYSCALL_QUERY_PMM_TOTAL => pmm_total_pages() as i64,
        SYSCALL_QUERY_PMM_FREE => pmm_free_pages() as i64,
        SYSCALL_QUERY_SCHED_COUNT => sched_count() as i64,
        SYSCALL_QUERY_VFS_COUNT => vfs_count() as i64,
        SYSCALL_QUERY_NET_QUEUE_LEN => net_queue_len() as i64,
        SYSCALL_QUERY_DRIVERS_COUNT => drivers_count() as i64,
        SYSCALL_QUERY_SYSCALL_COUNT => syscall_count as i64,
        SYSCALL_QUERY_PROC_COUNT => proc_count() as i64,
        _ => -1,
    }
}

fn syscall_register_default_queries() {
    let _ = syscall_register(
        SYSCALL_NR_QUERY_INIT_STATE,
        SYSCALL_OP_QUERY,
        SYSCALL_QUERY_INIT_STATE as i64,
    );
    let _ = syscall_register(
        SYSCALL_NR_QUERY_PMM_TOTAL,
        SYSCALL_OP_QUERY,
        SYSCALL_QUERY_PMM_TOTAL as i64,
    );
    let _ = syscall_register(
        SYSCALL_NR_QUERY_PMM_FREE,
        SYSCALL_OP_QUERY,
        SYSCALL_QUERY_PMM_FREE as i64,
    );
    let _ = syscall_register(
        SYSCALL_NR_QUERY_SCHED_COUNT,
        SYSCALL_OP_QUERY,
        SYSCALL_QUERY_SCHED_COUNT as i64,
    );
    let _ = syscall_register(
        SYSCALL_NR_QUERY_VFS_COUNT,
        SYSCALL_OP_QUERY,
        SYSCALL_QUERY_VFS_COUNT as i64,
    );
    let _ = syscall_register(
        SYSCALL_NR_QUERY_NET_QUEUE_LEN,
        SYSCALL_OP_QUERY,
        SYSCALL_QUERY_NET_QUEUE_LEN as i64,
    );
    let _ = syscall_register(
        SYSCALL_NR_QUERY_DRIVERS_COUNT,
        SYSCALL_OP_QUERY,
        SYSCALL_QUERY_DRIVERS_COUNT as i64,
    );
    let _ = syscall_register(
        SYSCALL_NR_QUERY_SYSCALL_COUNT,
        SYSCALL_OP_QUERY,
        SYSCALL_QUERY_SYSCALL_COUNT as i64,
    );
    let _ = syscall_register(
        SYSCALL_NR_QUERY_PROC_COUNT,
        SYSCALL_OP_QUERY,
        SYSCALL_QUERY_PROC_COUNT as i64,
    );
}

fn syscall_register_default_signal_ops() {
    let _ = syscall_register(SYSCALL_NR_FORK, SYSCALL_OP_FORK, 0);
    let _ = syscall_register(SYSCALL_NR_EXEC, SYSCALL_OP_EXEC, 0);
    let _ = syscall_register(SYSCALL_NR_EXIT, SYSCALL_OP_EXIT, 0);
    let _ = syscall_register(SYSCALL_NR_GETPID, SYSCALL_OP_GETPID, 0);
    let _ = syscall_register(SYSCALL_NR_WAITPID, SYSCALL_OP_WAITPID, 0);
    let _ = syscall_register(SYSCALL_NR_OPEN, SYSCALL_OP_OPEN, 0);
    let _ = syscall_register(SYSCALL_NR_READ, SYSCALL_OP_READ, 0);
    let _ = syscall_register(SYSCALL_NR_WRITE, SYSCALL_OP_WRITE, 0);
    let _ = syscall_register(SYSCALL_NR_CLOSE, SYSCALL_OP_CLOSE, 0);
    let _ = syscall_register(SYSCALL_NR_DUP2, SYSCALL_OP_DUP2, 0);
    let _ = syscall_register(SYSCALL_NR_MMAP, SYSCALL_OP_MMAP, 0);
    let _ = syscall_register(SYSCALL_NR_MUNMAP, SYSCALL_OP_MUNMAP, 0);
    let _ = syscall_register(SYSCALL_NR_YIELD, SYSCALL_OP_YIELD, 0);
    let _ = syscall_register(SYSCALL_NR_SLEEP, SYSCALL_OP_SLEEP, 0);
    let _ = syscall_register(SYSCALL_NR_SOCKET, SYSCALL_OP_SOCKET, 0);
    let _ = syscall_register(SYSCALL_NR_BIND, SYSCALL_OP_BIND, 0);
    let _ = syscall_register(SYSCALL_NR_LISTEN, SYSCALL_OP_LISTEN, 0);
    let _ = syscall_register(SYSCALL_NR_ACCEPT, SYSCALL_OP_ACCEPT, 0);
    let _ = syscall_register(SYSCALL_NR_CONNECT, SYSCALL_OP_CONNECT, 0);
    let _ = syscall_register(SYSCALL_NR_SEND, SYSCALL_OP_SEND, 0);
    let _ = syscall_register(SYSCALL_NR_RECV, SYSCALL_OP_RECV, 0);
    let _ = syscall_register(SYSCALL_NR_STAT, SYSCALL_OP_STAT, 0);
    let _ = syscall_register(SYSCALL_NR_MKDIR, SYSCALL_OP_MKDIR, 0);
    let _ = syscall_register(SYSCALL_NR_UNLINK, SYSCALL_OP_UNLINK, 0);
    let _ = syscall_register(SYSCALL_NR_GETDENTS, SYSCALL_OP_GETDENTS, 0);
    let _ = syscall_register(SYSCALL_NR_LSEEK, SYSCALL_OP_LSEEK, 0);
    let _ = syscall_register(SYSCALL_NR_PIPE, SYSCALL_OP_PIPE, 0);
    let _ = syscall_register(SYSCALL_NR_CHDIR, SYSCALL_OP_CHDIR, 0);
    let _ = syscall_register(SYSCALL_NR_GETCWD, SYSCALL_OP_GETCWD, 0);
    let _ = syscall_register(SYSCALL_NR_KILL, SYSCALL_OP_KILL, 0);
    let _ = syscall_register(SYSCALL_NR_SIGACTION, SYSCALL_OP_SIGACTION, 0);
    let _ = syscall_register(SYSCALL_NR_SIGPROCMASK, SYSCALL_OP_SIGPROCMASK, 0);
    let _ = syscall_register(SYSCALL_NR_SIGRETURN, SYSCALL_OP_SIGRETURN, 0);
    let _ = syscall_register(SYSCALL_NR_SIGPENDING, SYSCALL_OP_SIGPENDING, 0);
    let _ = syscall_register(SYSCALL_NR_SIGSUSPEND, SYSCALL_OP_SIGSUSPEND, 0);
    let _ = syscall_register(SYSCALL_NR_SIGALTSTACK, SYSCALL_OP_SIGALTSTACK, 0);
    let _ = syscall_register(SYSCALL_NR_DLOPEN, SYSCALL_OP_DLOPEN, 0);
    let _ = syscall_register(SYSCALL_NR_DLCLOSE, SYSCALL_OP_DLCLOSE, 0);
    let _ = syscall_register(SYSCALL_NR_DLSYM, SYSCALL_OP_DLSYM, 0);
    let _ = syscall_register(SYSCALL_NR_MODLOAD, SYSCALL_OP_MODLOAD, 0);
    let _ = syscall_register(SYSCALL_NR_MODUNLOAD, SYSCALL_OP_MODUNLOAD, 0);
    let _ = syscall_register(SYSCALL_NR_MODRELOAD, SYSCALL_OP_MODRELOAD, 0);
}

fn proc_init() {
    proc_reset();
    mark(QOS_INIT_PROC);
}

pub fn reset_init_state() {
    INIT_STATE.store(0, Ordering::SeqCst);
}

pub fn init_state() -> u32 {
    INIT_STATE.load(Ordering::SeqCst)
}

fn boot_info_valid(boot_info: &BootInfo) -> bool {
    if boot_info.magic != QOS_BOOT_MAGIC {
        return false;
    }

    if boot_info.mmap_entry_count == 0 || boot_info.mmap_entry_count > QOS_MMAP_MAX_ENTRIES as u32 {
        return false;
    }

    if boot_info.mmap_entries[0].length == 0 || boot_info.mmap_entries[0].type_ == 0 {
        return false;
    }

    if boot_info.initramfs_size != 0 && boot_info.initramfs_addr == 0 {
        return false;
    }

    true
}

pub fn pmm_reset() {
    mm_lock();
    *pmm_state_mut() = PmmState::new();
    mm_unlock();
}

fn pmm_add_range(state: &mut PmmState, base: u64, length: u64) {
    let mut start = align_up(base);
    let end = align_down(base.saturating_add(length));

    while start < end && (state.frame_count as usize) < QOS_PMM_MAX_PAGES {
        let idx = state.frame_count as usize;
        state.frames[idx] = start;
        state.used[idx] = 0;
        state.frame_count += 1;
        state.free_count += 1;
        start += QOS_PAGE_SIZE;
    }
}

fn pmm_reserve_range(state: &mut PmmState, base: u64, length: u64) {
    if length == 0 {
        return;
    }

    let end = base.saturating_add(length);
    let mut i = 0usize;
    while i < state.frame_count as usize {
        let frame = state.frames[i];
        if frame >= base && frame < end && state.used[i] == 0 {
            state.used[i] = 1;
            state.free_count -= 1;
        }
        i += 1;
    }
}

#[cfg(all(target_arch = "aarch64", target_os = "none"))]
unsafe extern "C" {
    static __kernel_phys_start: u8;
    static __kernel_phys_end: u8;
}

#[cfg(all(target_arch = "aarch64", target_os = "none"))]
fn pmm_reserve_kernel_image(state: &mut PmmState) {
    let start = unsafe { &__kernel_phys_start as *const u8 as u64 };
    let end = unsafe { &__kernel_phys_end as *const u8 as u64 };
    if start != 0 && end > start {
        pmm_reserve_range(state, start, end - start);
    }
}

#[cfg(not(all(target_arch = "aarch64", target_os = "none")))]
fn pmm_reserve_kernel_image(_state: &mut PmmState) {}

pub fn pmm_init_from_boot_info(boot_info: &BootInfo) -> Result<(), KernelError> {
    if !boot_info_valid(boot_info) {
        return Err(KernelError::InvalidBootInfo);
    }

    mm_lock();
    let result = (|| {
        let state = pmm_state_mut();
        *state = PmmState::new();

        let mut i = 0usize;
        while i < boot_info.mmap_entry_count as usize {
            let entry = boot_info.mmap_entries[i];
            if entry.type_ == 1 && entry.length >= QOS_PAGE_SIZE {
                pmm_add_range(state, entry.base, entry.length);
            }
            i += 1;
        }

        if state.frame_count == 0 {
            return Err(KernelError::InvalidBootInfo);
        }

        pmm_reserve_range(state, 0, 0x100000);
        pmm_reserve_kernel_image(state);
        if boot_info.initramfs_size != 0 {
            pmm_reserve_range(state, boot_info.initramfs_addr, boot_info.initramfs_size);
        }

        Ok(())
    })();
    mm_unlock();
    result
}

pub fn pmm_total_pages() -> u32 {
    mm_lock();
    let count = pmm_state().frame_count;
    mm_unlock();
    count
}

pub fn pmm_free_pages() -> u32 {
    mm_lock();
    let count = pmm_state().free_count;
    mm_unlock();
    count
}

pub fn pmm_alloc_page() -> Option<u64> {
    mm_lock();
    let mut out = None;
    let state = pmm_state_mut();
    let mut i = 0usize;
    while i < state.frame_count as usize {
        if state.used[i] == 0 {
            state.used[i] = 1;
            state.free_count -= 1;
            out = Some(state.frames[i]);
            break;
        }
        i += 1;
    }
    mm_unlock();
    out
}

pub fn pmm_free_page(page: u64) -> bool {
    if !is_page_aligned(page) {
        return false;
    }

    mm_lock();
    let mut ok = false;
    let state = pmm_state_mut();
    let mut i = 0usize;
    while i < state.frame_count as usize {
        if state.frames[i] == page {
            if state.used[i] != 0 {
                state.used[i] = 0;
                state.free_count += 1;
                ok = true;
            }
            break;
        }
        i += 1;
    }
    mm_unlock();
    ok
}

pub fn vmm_reset() {
    mm_lock();
    *vmm_state_mut() = VmmState::new();
    mm_unlock();
}

fn asid_valid(asid: u32) -> bool {
    (asid as usize) < QOS_VMM_MAX_AS
}

fn vmm_find_as(state: &VmmState, asid: u32, page_va: u64) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_VMM_MAX_MAPPINGS {
        if state.used[i] != 0 && state.asids[i] == asid && state.vas[i] == page_va {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn vmm_free_slot(state: &VmmState) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_VMM_MAX_MAPPINGS {
        if state.used[i] == 0 {
            return Some(i);
        }
        i += 1;
    }
    None
}

pub fn vmm_set_asid(asid: u32) {
    if !asid_valid(asid) {
        return;
    }
    mm_lock();
    vmm_state_mut().current_asid = asid;
    mm_unlock();
}

pub fn vmm_get_asid() -> u32 {
    mm_lock();
    let asid = vmm_state().current_asid;
    mm_unlock();
    asid
}

pub fn vmm_map_as(asid: u32, va: u64, pa: u64, flags: u32) -> Result<(), KernelError> {
    if !asid_valid(asid) || flags == 0 || !is_page_aligned(va) || !is_page_aligned(pa) {
        return Err(KernelError::InvalidAddress);
    }

    mm_lock();
    let result = (|| {
        let state = vmm_state_mut();
        let page_va = align_down(va);
        let idx = if let Some(found) = vmm_find_as(state, asid, page_va) {
            found
        } else if let Some(slot) = vmm_free_slot(state) {
            state.used[slot] = 1;
            state.asids[slot] = asid;
            state.vas[slot] = page_va;
            slot
        } else {
            return Err(KernelError::VmmFull);
        };

        state.pas[idx] = pa;
        state.flags[idx] = flags;
        #[cfg(all(feature = "mm-debug", test))]
        std::eprintln!(
            "[mm_debug] vmm_map_as: asid={} VA={:#x} -> PA={:#x} PFN={:#x} flags={:#x}",
            asid, va, pa, pa >> 12, flags
        );
        Ok(())
    })();
    mm_unlock();
    result
}

pub fn vmm_unmap_as(asid: u32, va: u64) -> bool {
    if !asid_valid(asid) || !is_page_aligned(va) {
        return false;
    }

    mm_lock();
    let ok = (|| {
        let state = vmm_state_mut();
        let page_va = align_down(va);
        if let Some(idx) = vmm_find_as(state, asid, page_va) {
            state.used[idx] = 0;
            state.asids[idx] = 0;
            state.vas[idx] = 0;
            state.pas[idx] = 0;
            state.flags[idx] = 0;
            true
        } else {
            false
        }
    })();
    mm_unlock();
    ok
}

pub fn vmm_unmap(va: u64) -> bool {
    mm_lock();
    let asid = vmm_state().current_asid;
    mm_unlock();
    vmm_unmap_as(asid, va)
}

pub fn vmm_translate_as(asid: u32, va: u64) -> Option<u64> {
    if !asid_valid(asid) {
        return None;
    }
    mm_lock();
    let out = (|| {
        let state = vmm_state();
        let page_va = align_down(va);
        let offset = va & (QOS_PAGE_SIZE - 1);
        vmm_find_as(state, asid, page_va).map(|idx| state.pas[idx] + offset)
    })();
    mm_unlock();
    out
}

pub fn vmm_translate(va: u64) -> Option<u64> {
    mm_lock();
    let asid = vmm_state().current_asid;
    mm_unlock();
    vmm_translate_as(asid, va)
}

pub fn vmm_flags_as(asid: u32, va: u64) -> u32 {
    if !asid_valid(asid) {
        return 0;
    }
    mm_lock();
    let out = (|| {
        let state = vmm_state();
        let page_va = align_down(va);
        if let Some(idx) = vmm_find_as(state, asid, page_va) {
            state.flags[idx]
        } else {
            0
        }
    })();
    mm_unlock();
    out
}

pub fn vmm_flags(va: u64) -> u32 {
    mm_lock();
    let asid = vmm_state().current_asid;
    mm_unlock();
    vmm_flags_as(asid, va)
}

pub fn vmm_mapping_count_as(asid: u32) -> u32 {
    if !asid_valid(asid) {
        return 0;
    }
    mm_lock();
    let count = {
        let state = vmm_state();
        let mut i = 0usize;
        let mut n = 0u32;
        while i < QOS_VMM_MAX_MAPPINGS {
            if state.used[i] != 0 && state.asids[i] == asid {
                n = n.saturating_add(1);
            }
            i += 1;
        }
        n
    };
    mm_unlock();
    count
}

pub fn vmm_mapping_count() -> u32 {
    mm_lock();
    let asid = vmm_state().current_asid;
    mm_unlock();
    vmm_mapping_count_as(asid)
}

pub fn vmm_mapping_get_as(asid: u32, ordinal: u32) -> Option<(u64, u64, u32)> {
    if !asid_valid(asid) {
        return None;
    }
    mm_lock();
    let out = {
        let state = vmm_state();
        let mut i = 0usize;
        let mut seen = 0u32;
        let mut map = None;
        while i < QOS_VMM_MAX_MAPPINGS {
            if state.used[i] != 0 && state.asids[i] == asid {
                if seen == ordinal {
                    map = Some((state.vas[i], state.pas[i], state.flags[i]));
                    break;
                }
                seen = seen.saturating_add(1);
            }
            i += 1;
        }
        map
    };
    mm_unlock();
    out
}

pub fn vmm_mapping_get(ordinal: u32) -> Option<(u64, u64, u32)> {
    mm_lock();
    let asid = vmm_state().current_asid;
    mm_unlock();
    vmm_mapping_get_as(asid, ordinal)
}

pub fn vmm_map(va: u64, pa: u64, flags: u32) -> Result<(), KernelError> {
    if flags == 0 || !is_page_aligned(va) || !is_page_aligned(pa) {
        return Err(KernelError::InvalidAddress);
    }

    mm_lock();
    let asid = vmm_state().current_asid;
    mm_unlock();
    vmm_map_as(asid, va, pa, flags)
}

pub fn sched_reset() {
    sched_lock();
    *sched_state_mut() = SchedState::new();
    sched_unlock();
}

pub fn sched_add_task(pid: u32) -> bool {
    if pid == 0 {
        return false;
    }

    sched_lock();
    let ok = (|| {
        let state = sched_state_mut();
        let mut i = 0usize;
        while i < state.count as usize {
            if state.pids[i] == pid {
                return false;
            }
            i += 1;
        }

        if state.count as usize >= QOS_SCHED_MAX_TASKS {
            return false;
        }

        state.pids[state.count as usize] = pid;
        state.count += 1;
        true
    })();
    sched_unlock();
    ok
}

pub fn sched_remove_task(pid: u32) -> bool {
    sched_lock();
    let ok = (|| {
        let state = sched_state_mut();
        let mut i = 0usize;
        while i < state.count as usize {
            if state.pids[i] == pid {
                break;
            }
            i += 1;
        }

        if i >= state.count as usize {
            return false;
        }

        while i + 1 < state.count as usize {
            state.pids[i] = state.pids[i + 1];
            i += 1;
        }

        state.count -= 1;
        state.pids[state.count as usize] = 0;
        if state.current_pid == pid {
            state.current_pid = 0;
        }
        if state.count == 0 || state.cursor >= state.count {
            state.cursor = 0;
        }
        true
    })();
    sched_unlock();
    ok
}

pub fn sched_count() -> u32 {
    sched_lock();
    let count = sched_state().count;
    sched_unlock();
    count
}

pub fn sched_next() -> Option<u32> {
    sched_lock();
    let next = (|| {
        let state = sched_state_mut();
        if state.count == 0 {
            state.current_pid = 0;
            return None;
        }

        let idx = state.cursor as usize;
        let pid = state.pids[idx];
        state.cursor = (state.cursor + 1) % state.count;
        state.current_pid = pid;
        Some(pid)
    })();
    sched_unlock();
    next
}

pub fn sched_current() -> u32 {
    sched_lock();
    let pid = sched_state().current_pid;
    sched_unlock();
    pid
}

fn vfs_path_valid(path: &str) -> bool {
    let bytes = path.as_bytes();
    if bytes.is_empty() || bytes[0] != b'/' {
        return false;
    }
    bytes.len() < QOS_VFS_PATH_MAX
}

fn vfs_is_mount_or_child(path: &str, mount: &str) -> bool {
    if !path.starts_with(mount) {
        return false;
    }
    if path.len() == mount.len() {
        return true;
    }
    path.as_bytes()[mount.len()] == b'/'
}

fn vfs_find(state: &VfsState, path: &str) -> Option<usize> {
    let bytes = path.as_bytes();
    let mut i = 0usize;
    while i < QOS_VFS_MAX_NODES {
        if state.used[i] != 0
            && state.lens[i] as usize == bytes.len()
            && &state.paths[i][..bytes.len()] == bytes
        {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn vfs_free_slot(state: &VfsState) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_VFS_MAX_NODES {
        if state.used[i] == 0 {
            return Some(i);
        }
        i += 1;
    }
    None
}

pub fn vfs_reset() {
    vfs_lock();
    *vfs_state_mut() = VfsState::new();
    vfs_unlock();
}

pub fn vfs_create(path: &str) -> bool {
    if !vfs_path_valid(path) {
        return false;
    }

    vfs_lock();
    let ok = (|| {
        let state = vfs_state_mut();
        if vfs_find(state, path).is_some() {
            return false;
        }
        let slot = match vfs_free_slot(state) {
            Some(s) => s,
            None => return false,
        };

        let bytes = path.as_bytes();
        let mut i = 0usize;
        while i < bytes.len() {
            state.paths[slot][i] = bytes[i];
            i += 1;
        }
        state.lens[slot] = bytes.len() as u8;
        state.used[slot] = 1;
        state.count += 1;
        true
    })();
    vfs_unlock();
    ok
}

pub fn vfs_exists(path: &str) -> bool {
    if !vfs_path_valid(path) {
        return false;
    }

    vfs_lock();
    let exists = vfs_find(vfs_state(), path).is_some();
    vfs_unlock();
    exists
}

pub fn vfs_remove(path: &str) -> bool {
    if !vfs_path_valid(path) {
        return false;
    }

    vfs_lock();
    let ok = (|| {
        let state = vfs_state_mut();
        let idx = match vfs_find(state, path) {
            Some(i) => i,
            None => return false,
        };
        state.used[idx] = 0;
        state.lens[idx] = 0;
        state.paths[idx][0] = 0;
        state.count -= 1;
        true
    })();
    vfs_unlock();
    ok
}

pub fn vfs_count() -> u32 {
    vfs_lock();
    let count = vfs_state().count;
    vfs_unlock();
    count
}

pub fn vfs_fs_kind(path: &str) -> Option<u32> {
    if !vfs_path_valid(path) {
        return None;
    }
    if vfs_is_mount_or_child(path, "/tmp") {
        return Some(QOS_VFS_FS_TMPFS);
    }
    if vfs_is_mount_or_child(path, "/proc") {
        return Some(QOS_VFS_FS_PROCFS);
    }
    if vfs_is_mount_or_child(path, "/data") {
        return Some(QOS_VFS_FS_EXT2);
    }
    Some(QOS_VFS_FS_INITRAMFS)
}

pub fn vfs_is_read_only(path: &str) -> Option<bool> {
    let kind = vfs_fs_kind(path)?;
    Some(kind == QOS_VFS_FS_INITRAMFS || kind == QOS_VFS_FS_PROCFS)
}

pub fn net_reset() {
    net_lock();
    zero_struct(net_state_mut());
    net_unlock();
}

fn net_same_subnet(a: u32, b: u32) -> bool {
    (a & QOS_NET_IPV4_SUBNET_MASK) == (b & QOS_NET_IPV4_SUBNET_MASK)
}

fn net_udp_port_bound(state: &NetState, port: u16) -> bool {
    let mut i = 0usize;
    while i < QOS_NET_UDP_PORT_MAX {
        if state.udp_port_used[i] != 0 && state.udp_ports[i] == port {
            return true;
        }
        i += 1;
    }
    false
}

fn net_udp_drop_datagrams_for_port(state: &mut NetState, port: u16) {
    let mut src = 0usize;
    let mut dst = 0usize;
    while src < state.udp_count as usize {
        if state.udp_used[src] != 0 && state.udp_dst_port[src] == port {
            src += 1;
            continue;
        }
        if dst != src {
            state.udp_used[dst] = state.udp_used[src];
            state.udp_dst_port[dst] = state.udp_dst_port[src];
            state.udp_src_port[dst] = state.udp_src_port[src];
            state.udp_src_ip[dst] = state.udp_src_ip[src];
            state.udp_len[dst] = state.udp_len[src];
            state.udp_data[dst] = state.udp_data[src];
        }
        dst += 1;
        src += 1;
    }
    while dst < state.udp_count as usize {
        state.udp_used[dst] = 0;
        state.udp_dst_port[dst] = 0;
        state.udp_src_port[dst] = 0;
        state.udp_src_ip[dst] = 0;
        state.udp_len[dst] = 0;
        state.udp_data[dst] = [0; QOS_NET_MAX_PACKET];
        dst += 1;
    }
    state.udp_count = dst as u32;
}

fn net_tcp_listener_exists(state: &NetState, port: u16) -> bool {
    let mut i = 0usize;
    while i < QOS_NET_TCP_MAX_LISTENERS {
        if state.tcp_listener_used[i] != 0 && state.tcp_listeners[i] == port {
            return true;
        }
        i += 1;
    }
    false
}

fn net_tcp_next_rto(rto_ms: u32) -> u32 {
    if rto_ms >= 60000 {
        return 60000;
    }
    if rto_ms > 30000 {
        return 60000;
    }
    rto_ms.saturating_mul(2)
}

pub fn net_tcp_next_rto_ms(current_rto_ms: u32) -> u32 {
    net_tcp_next_rto(current_rto_ms)
}

pub fn net_enqueue_packet(data: &[u8]) -> bool {
    if data.is_empty() || data.len() > QOS_NET_MAX_PACKET {
        return false;
    }

    net_lock();
    let ok = (|| {
        let state = net_state_mut();
        if state.count as usize >= QOS_NET_MAX_QUEUE {
            return false;
        }

        let slot = state.tail as usize;
        let mut i = 0usize;
        while i < data.len() {
            state.packets[slot][i] = data[i];
            i += 1;
        }
        state.lens[slot] = data.len() as u16;
        state.tail = (state.tail + 1) % QOS_NET_MAX_QUEUE as u32;
        state.count += 1;
        true
    })();
    net_unlock();
    ok
}

pub fn net_dequeue_packet(out: &mut [u8]) -> Option<usize> {
    if out.is_empty() {
        return None;
    }

    net_lock();
    let len = (|| {
        let state = net_state_mut();
        if state.count == 0 {
            return None;
        }

        let slot = state.head as usize;
        let packet_len = state.lens[slot] as usize;
        if out.len() < packet_len {
            return None;
        }

        let mut i = 0usize;
        while i < packet_len {
            out[i] = state.packets[slot][i];
            i += 1;
        }

        state.lens[slot] = 0;
        state.head = (state.head + 1) % QOS_NET_MAX_QUEUE as u32;
        state.count -= 1;
        Some(packet_len)
    })();
    net_unlock();
    len
}

pub fn net_queue_len() -> u32 {
    net_lock();
    let n = net_state().count;
    net_unlock();
    n
}

pub fn net_arp_put(ip: u32, mac: &[u8; 6], ttl_secs: u32) -> bool {
    if ip == 0 || ttl_secs == 0 {
        return false;
    }
    net_lock();
    let ok = (|| {
        let state = net_state_mut();
        let mut i = 0usize;
        let mut free_slot: Option<usize> = None;
        while i < QOS_NET_ARP_MAX {
            if state.arp_used[i] != 0 && state.arp_ip[i] == ip {
                state.arp_mac[i] = *mac;
                state.arp_ttl_secs[i] = ttl_secs;
                return true;
            }
            if free_slot.is_none() && state.arp_used[i] == 0 {
                free_slot = Some(i);
            }
            i += 1;
        }
        let idx = match free_slot {
            Some(v) => v,
            None => return false,
        };
        state.arp_used[idx] = 1;
        state.arp_ip[idx] = ip;
        state.arp_mac[idx] = *mac;
        state.arp_ttl_secs[idx] = ttl_secs;
        true
    })();
    net_unlock();
    ok
}

pub fn net_arp_lookup(ip: u32) -> Option<[u8; 6]> {
    if ip == 0 {
        return None;
    }
    net_lock();
    let out = (|| {
        let state = net_state();
        let mut i = 0usize;
        while i < QOS_NET_ARP_MAX {
            if state.arp_used[i] != 0 && state.arp_ip[i] == ip && state.arp_ttl_secs[i] != 0 {
                return Some(state.arp_mac[i]);
            }
            i += 1;
        }
        None
    })();
    net_unlock();
    out
}

pub fn net_arp_tick(elapsed_secs: u32) {
    if elapsed_secs == 0 {
        return;
    }
    net_lock();
    {
        let state = net_state_mut();
        let mut i = 0usize;
        while i < QOS_NET_ARP_MAX {
            if state.arp_used[i] != 0 {
                if state.arp_ttl_secs[i] <= elapsed_secs {
                    state.arp_used[i] = 0;
                    state.arp_ip[i] = 0;
                    state.arp_mac[i] = [0; 6];
                    state.arp_ttl_secs[i] = 0;
                } else {
                    state.arp_ttl_secs[i] -= elapsed_secs;
                }
            }
            i += 1;
        }
    }
    net_unlock();
}

pub fn net_arp_count() -> u32 {
    net_lock();
    let count = (|| {
        let state = net_state();
        let mut i = 0usize;
        let mut n = 0u32;
        while i < QOS_NET_ARP_MAX {
            if state.arp_used[i] != 0 {
                n += 1;
            }
            i += 1;
        }
        n
    })();
    net_unlock();
    count
}

pub fn net_ipv4_route(dst_ip: u32) -> Option<u32> {
    if dst_ip == 0 {
        return None;
    }
    if net_same_subnet(dst_ip, QOS_NET_IPV4_LOCAL) {
        Some(dst_ip)
    } else {
        Some(QOS_NET_IPV4_GATEWAY)
    }
}

pub fn net_icmp_echo(
    dst_ip: u32,
    ident: u16,
    seq: u16,
    payload: &[u8],
    out_reply: &mut [u8],
) -> Option<(usize, u32)> {
    let _ = ident;
    let _ = seq;
    if dst_ip == 0 || payload.is_empty() || payload.len() > QOS_NET_MAX_PACKET || out_reply.len() < payload.len() {
        return None;
    }
    let _ = net_ipv4_route(dst_ip)?;
    if dst_ip != QOS_NET_IPV4_GATEWAY && dst_ip != QOS_NET_IPV4_LOCAL {
        return None;
    }
    let n = payload.len();
    out_reply[..n].copy_from_slice(payload);
    Some((n, dst_ip))
}

pub fn net_udp_bind(port: u16) -> bool {
    if port < QOS_NET_PORT_MIN_DYNAMIC {
        return false;
    }
    net_lock();
    let ok = (|| {
        let state = net_state_mut();
        if net_udp_port_bound(state, port) {
            return false;
        }
        let mut i = 0usize;
        while i < QOS_NET_UDP_PORT_MAX {
            if state.udp_port_used[i] == 0 {
                state.udp_port_used[i] = 1;
                state.udp_ports[i] = port;
                return true;
            }
            i += 1;
        }
        false
    })();
    net_unlock();
    ok
}

pub fn net_udp_unbind(port: u16) -> bool {
    if port < QOS_NET_PORT_MIN_DYNAMIC {
        return false;
    }
    net_lock();
    let ok = (|| {
        let state = net_state_mut();
        let mut i = 0usize;
        while i < QOS_NET_UDP_PORT_MAX {
            if state.udp_port_used[i] != 0 && state.udp_ports[i] == port {
                state.udp_port_used[i] = 0;
                state.udp_ports[i] = 0;
                net_udp_drop_datagrams_for_port(state, port);
                return true;
            }
            i += 1;
        }
        false
    })();
    net_unlock();
    ok
}

pub fn net_udp_send(src_port: u16, src_ip: u32, dst_port: u16, data: &[u8]) -> Option<usize> {
    if src_port < QOS_NET_PORT_MIN_DYNAMIC
        || src_ip == 0
        || dst_port == 0
        || data.is_empty()
        || data.len() > QOS_NET_MAX_PACKET
    {
        return None;
    }
    net_lock();
    let sent = (|| {
        let state = net_state_mut();
        if !net_udp_port_bound(state, dst_port) || state.udp_count as usize >= QOS_NET_UDP_MAX_DATAGRAMS {
            return None;
        }
        let slot = state.udp_count as usize;
        state.udp_used[slot] = 1;
        state.udp_dst_port[slot] = dst_port;
        state.udp_src_port[slot] = src_port;
        state.udp_src_ip[slot] = src_ip;
        state.udp_len[slot] = data.len() as u16;
        state.udp_data[slot][..data.len()].copy_from_slice(data);
        state.udp_count += 1;
        Some(data.len())
    })();
    net_unlock();
    sent
}

pub fn net_udp_recv(dst_port: u16, out: &mut [u8]) -> Option<(usize, u32, u16)> {
    if dst_port == 0 || out.is_empty() {
        return None;
    }
    net_lock();
    let got = (|| {
        let state = net_state_mut();
        let mut i = 0usize;
        while i < state.udp_count as usize {
            if state.udp_used[i] != 0 && state.udp_dst_port[i] == dst_port {
                let len = state.udp_len[i] as usize;
                if out.len() < len {
                    return None;
                }
                out[..len].copy_from_slice(&state.udp_data[i][..len]);
                let src_ip = state.udp_src_ip[i];
                let src_port = state.udp_src_port[i];
                let mut j = i;
                while j + 1 < state.udp_count as usize {
                    state.udp_used[j] = state.udp_used[j + 1];
                    state.udp_dst_port[j] = state.udp_dst_port[j + 1];
                    state.udp_src_port[j] = state.udp_src_port[j + 1];
                    state.udp_src_ip[j] = state.udp_src_ip[j + 1];
                    state.udp_len[j] = state.udp_len[j + 1];
                    state.udp_data[j] = state.udp_data[j + 1];
                    j += 1;
                }
                let tail = (state.udp_count as usize).saturating_sub(1);
                state.udp_used[tail] = 0;
                state.udp_dst_port[tail] = 0;
                state.udp_src_port[tail] = 0;
                state.udp_src_ip[tail] = 0;
                state.udp_len[tail] = 0;
                state.udp_data[tail] = [0; QOS_NET_MAX_PACKET];
                state.udp_count -= 1;
                return Some((len, src_ip, src_port));
            }
            i += 1;
        }
        None
    })();
    net_unlock();
    got
}

pub fn net_tcp_listen(port: u16) -> Option<u16> {
    if port < QOS_NET_PORT_MIN_DYNAMIC {
        return None;
    }
    net_lock();
    let id = (|| {
        let state = net_state_mut();
        if net_tcp_listener_exists(state, port) {
            return None;
        }
        let mut i = 0usize;
        while i < QOS_NET_TCP_MAX_LISTENERS {
            if state.tcp_listener_used[i] == 0 {
                state.tcp_listener_used[i] = 1;
                state.tcp_listeners[i] = port;
                return Some(i as u16);
            }
            i += 1;
        }
        None
    })();
    net_unlock();
    id
}

pub fn net_tcp_unlisten(port: u16) -> bool {
    if port < QOS_NET_PORT_MIN_DYNAMIC {
        return false;
    }
    net_lock();
    let ok = (|| {
        let state = net_state_mut();
        let mut i = 0usize;
        while i < QOS_NET_TCP_MAX_LISTENERS {
            if state.tcp_listener_used[i] != 0 && state.tcp_listeners[i] == port {
                state.tcp_listener_used[i] = 0;
                state.tcp_listeners[i] = 0;
                return true;
            }
            i += 1;
        }
        false
    })();
    net_unlock();
    ok
}

pub fn net_tcp_connect(local_port: u16, remote_ip: u32, remote_port: u16) -> Option<u16> {
    if local_port < QOS_NET_PORT_MIN_DYNAMIC || remote_ip == 0 || remote_port == 0 {
        return None;
    }
    net_lock();
    let id = (|| {
        let state = net_state_mut();
        if !net_tcp_listener_exists(state, remote_port) {
            return None;
        }
        let mut i = 0usize;
        while i < QOS_NET_TCP_MAX_CONNS {
            if state.tcp_conn_used[i] == 0 {
                state.tcp_conn_used[i] = 1;
                state.tcp_conn_state[i] = QOS_TCP_STATE_SYN_SENT as u8;
                state.tcp_local_port[i] = local_port;
                state.tcp_remote_port[i] = remote_port;
                state.tcp_remote_ip[i] = remote_ip;
                state.tcp_rto_ms[i] = 500;
                state.tcp_retries[i] = 0;
                return Some(i as u16);
            }
            i += 1;
        }
        None
    })();
    net_unlock();
    id
}

pub fn net_tcp_step(conn_id: u16, event: u32) -> bool {
    net_lock();
    let ok = (|| {
        let state = net_state_mut();
        let idx = conn_id as usize;
        if idx >= QOS_NET_TCP_MAX_CONNS || state.tcp_conn_used[idx] == 0 {
            return false;
        }
        let cur = state.tcp_conn_state[idx] as u32;
        if event == QOS_TCP_EVT_TIMEOUT {
            if cur == QOS_TCP_STATE_TIME_WAIT {
                state.tcp_conn_state[idx] = QOS_TCP_STATE_CLOSED as u8;
                return true;
            }
            if cur == QOS_TCP_STATE_SYN_SENT {
                if state.tcp_retries[idx] >= 5 {
                    state.tcp_conn_state[idx] = QOS_TCP_STATE_CLOSED as u8;
                } else {
                    state.tcp_retries[idx] = state.tcp_retries[idx].saturating_add(1);
                    state.tcp_rto_ms[idx] = net_tcp_next_rto(state.tcp_rto_ms[idx]);
                }
                return true;
            }
        }

        if cur == QOS_TCP_STATE_LISTEN && event == QOS_TCP_EVT_RX_SYN {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_SYN_RECEIVED as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_SYN_SENT && event == QOS_TCP_EVT_RX_SYN_ACK {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_ESTABLISHED as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_SYN_RECEIVED && event == QOS_TCP_EVT_RX_ACK {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_ESTABLISHED as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_ESTABLISHED && event == QOS_TCP_EVT_APP_CLOSE {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_FIN_WAIT_1 as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_ESTABLISHED && event == QOS_TCP_EVT_RX_FIN {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_CLOSE_WAIT as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_FIN_WAIT_1 && event == QOS_TCP_EVT_RX_ACK {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_FIN_WAIT_2 as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_FIN_WAIT_1 && event == QOS_TCP_EVT_RX_FIN {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_CLOSING as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_FIN_WAIT_2 && event == QOS_TCP_EVT_RX_FIN {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_TIME_WAIT as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_CLOSE_WAIT && event == QOS_TCP_EVT_APP_CLOSE {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_LAST_ACK as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_CLOSING && event == QOS_TCP_EVT_RX_ACK {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_TIME_WAIT as u8;
            return true;
        }
        if cur == QOS_TCP_STATE_LAST_ACK && event == QOS_TCP_EVT_RX_ACK {
            state.tcp_conn_state[idx] = QOS_TCP_STATE_CLOSED as u8;
            return true;
        }
        false
    })();
    net_unlock();
    ok
}

pub fn net_tcp_state(conn_id: u16) -> Option<u32> {
    net_lock();
    let state = (|| {
        let s = net_state();
        let idx = conn_id as usize;
        if idx >= QOS_NET_TCP_MAX_CONNS || s.tcp_conn_used[idx] == 0 {
            return None;
        }
        Some(s.tcp_conn_state[idx] as u32)
    })();
    net_unlock();
    state
}

pub fn net_tcp_rto(conn_id: u16) -> Option<u32> {
    net_lock();
    let rto = (|| {
        let s = net_state();
        let idx = conn_id as usize;
        if idx >= QOS_NET_TCP_MAX_CONNS || s.tcp_conn_used[idx] == 0 {
            return None;
        }
        Some(s.tcp_rto_ms[idx])
    })();
    net_unlock();
    rto
}

pub fn net_tcp_retries(conn_id: u16) -> Option<u32> {
    net_lock();
    let retries = (|| {
        let s = net_state();
        let idx = conn_id as usize;
        if idx >= QOS_NET_TCP_MAX_CONNS || s.tcp_conn_used[idx] == 0 {
            return None;
        }
        Some(s.tcp_retries[idx] as u32)
    })();
    net_unlock();
    retries
}

fn softirq_vector_valid(vector: u32) -> bool {
    (vector as usize) < QOS_SOFTIRQ_MAX
}

pub fn softirq_reset() {
    softirq_lock();
    *softirq_state_mut() = SoftirqState::new();
    softirq_unlock();
}

pub fn softirq_register(vector: u32, handler: SoftirqHandler, ctx: usize) -> bool {
    if !softirq_vector_valid(vector) {
        return false;
    }
    softirq_lock();
    {
        let state = softirq_state_mut();
        state.handlers[vector as usize] = Some(handler);
        state.handler_ctx[vector as usize] = ctx;
    }
    softirq_unlock();
    true
}

pub fn softirq_raise(vector: u32) -> bool {
    if !softirq_vector_valid(vector) {
        return false;
    }
    softirq_lock();
    {
        let state = softirq_state_mut();
        state.pending_mask |= 1u32 << vector;
    }
    softirq_unlock();
    true
}

pub fn softirq_pending_mask() -> u32 {
    softirq_lock();
    let pending = softirq_state().pending_mask;
    softirq_unlock();
    pending
}

pub fn softirq_run() -> u32 {
    let mut handled = 0u32;
    loop {
        let pending = softirq_pending_mask();
        if pending == 0 {
            break;
        }

        let mut vector: Option<usize> = None;
        let mut i = 0usize;
        while i < QOS_SOFTIRQ_MAX {
            if (pending & (1u32 << i)) != 0 {
                vector = Some(i);
                break;
            }
            i += 1;
        }

        let v = match vector {
            Some(idx) => idx,
            None => break,
        };

        let (handler, ctx) = {
            softirq_lock();
            let state = softirq_state_mut();
            state.pending_mask &= !(1u32 << v);
            (state.handlers[v], state.handler_ctx[v])
        };
        softirq_unlock();

        if let Some(fn_ptr) = handler {
            fn_ptr(ctx);
            handled = handled.saturating_add(1);
        }
    }
    handled
}

fn timer_id_valid(timer_id: i32) -> bool {
    timer_id > 0 && (timer_id as usize) <= QOS_TIMER_MAX
}

fn timer_softirq_handler(_ctx: usize) {
    loop {
        let task = {
            timer_lock();
            let state = timer_state_mut();
            let now = state.jiffies;
            let mut selected: Option<(TimerCallback, usize)> = None;
            let mut i = 0usize;
            while i < QOS_TIMER_MAX {
                let entry = &mut state.entries[i];
                if entry.used != 0 && entry.pending != 0 && entry.expires <= now {
                    let cb = match entry.callback {
                        Some(v) => v,
                        None => {
                            entry.pending = 0;
                            i += 1;
                            continue;
                        }
                    };
                    let ctx = entry.ctx;
                    entry.pending = 0;
                    if entry.periodic != 0 && entry.interval != 0 {
                        let step = entry.interval as u64;
                        while entry.expires <= now {
                            entry.expires = entry.expires.saturating_add(step);
                        }
                    } else {
                        *entry = TimerEntry::new();
                    }
                    selected = Some((cb, ctx));
                    break;
                }
                i += 1;
            }
            selected
        };
        timer_unlock();

        let (cb, ctx) = match task {
            Some(v) => v,
            None => break,
        };
        cb(ctx);
    }
}

pub fn timer_reset() {
    timer_lock();
    *timer_state_mut() = TimerState::new();
    timer_unlock();
}

pub fn timer_jiffies() -> u64 {
    timer_lock();
    let now = timer_state().jiffies;
    timer_unlock();
    now
}

pub fn timer_add(delay_ticks: u32, interval_ticks: u32, callback: TimerCallback, ctx: usize) -> Option<i32> {
    if delay_ticks == 0 {
        return None;
    }
    timer_lock();
    let out = {
        let state = timer_state_mut();
        let mut i = 0usize;
        let mut timer_id = None;
        while i < QOS_TIMER_MAX {
            if state.entries[i].used == 0 {
                state.entries[i].used = 1;
                state.entries[i].periodic = if interval_ticks == 0 { 0 } else { 1 };
                state.entries[i].pending = 0;
                state.entries[i].expires = state.jiffies.saturating_add(delay_ticks as u64);
                state.entries[i].interval = interval_ticks;
                state.entries[i].callback = Some(callback);
                state.entries[i].ctx = ctx;
                timer_id = Some((i + 1) as i32);
                break;
            }
            i += 1;
        }
        timer_id
    };
    timer_unlock();
    out
}

pub fn timer_cancel(timer_id: i32) -> bool {
    if !timer_id_valid(timer_id) {
        return false;
    }
    timer_lock();
    let ok = {
        let state = timer_state_mut();
        let idx = (timer_id - 1) as usize;
        if state.entries[idx].used == 0 {
            false
        } else {
            state.entries[idx] = TimerEntry::new();
            true
        }
    };
    timer_unlock();
    ok
}

pub fn timer_tick(elapsed_ticks: u32) {
    if elapsed_ticks == 0 {
        return;
    }
    let need_softirq = {
        timer_lock();
        let state = timer_state_mut();
        state.jiffies = state.jiffies.saturating_add(elapsed_ticks as u64);
        let now = state.jiffies;
        let mut need = false;
        let mut i = 0usize;
        while i < QOS_TIMER_MAX {
            let entry = &mut state.entries[i];
            if entry.used != 0 && entry.pending == 0 && entry.expires <= now {
                entry.pending = 1;
                need = true;
            }
            i += 1;
        }
        need
    };
    timer_unlock();
    if need_softirq {
        let _ = softirq_raise(QOS_SOFTIRQ_TIMER);
    }
}

pub fn timer_active_count() -> u32 {
    timer_lock();
    let count = {
        let state = timer_state();
        let mut i = 0usize;
        let mut n = 0u32;
        while i < QOS_TIMER_MAX {
            if state.entries[i].used != 0 {
                n = n.saturating_add(1);
            }
            i += 1;
        }
        n
    };
    timer_unlock();
    count
}

pub fn timer_pending_count() -> u32 {
    timer_lock();
    let count = {
        let state = timer_state();
        let mut i = 0usize;
        let mut n = 0u32;
        while i < QOS_TIMER_MAX {
            if state.entries[i].used != 0 && state.entries[i].pending != 0 {
                n = n.saturating_add(1);
            }
            i += 1;
        }
        n
    };
    timer_unlock();
    count
}

fn kthread_find_slot(state: &KthreadState, tid: u32) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_KTHREAD_MAX {
        let entry = &state.entries[i];
        if entry.used != 0 && entry.tid == tid {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn kthread_name_init_slot(state: &mut KthreadState, slot: usize, tid: u32) {
    let prefix = b"kthread-";
    let mut off = 0usize;
    while off < prefix.len() && off < (QOS_KTHREAD_NAME_MAX.saturating_sub(1)) {
        state.names[slot][off] = prefix[off];
        off += 1;
    }
    if off < (QOS_KTHREAD_NAME_MAX.saturating_sub(1)) {
        off += write_u32_ascii(&mut state.names[slot][off..QOS_KTHREAD_NAME_MAX - 1], tid);
    }
    if off >= QOS_KTHREAD_NAME_MAX {
        off = QOS_KTHREAD_NAME_MAX - 1;
    }
    state.names[slot][off] = 0;
    state.name_len[slot] = off as u8;
}

pub fn kthread_reset() {
    kthread_lock();
    *kthread_state_mut() = KthreadState::new();
    kthread_unlock();
}

pub fn kthread_create(entry: KthreadFn, arg: usize) -> Option<u32> {
    kthread_lock();
    let tid = {
        let state = kthread_state_mut();
        let mut i = 0usize;
        let mut out = None;
        while i < QOS_KTHREAD_MAX {
            if state.entries[i].used == 0 {
                let mut tid = state.next_tid;
                state.next_tid = state.next_tid.wrapping_add(1);
                if tid == 0 {
                    tid = state.next_tid;
                    state.next_tid = state.next_tid.wrapping_add(1);
                }
                state.entries[i].used = 1;
                state.entries[i].runnable = 1;
                state.entries[i].tid = tid;
                state.entries[i].entry = Some(entry);
                state.entries[i].arg = arg;
                state.entries[i].run_count = 0;
                kthread_name_init_slot(state, i, tid);
                out = Some(tid);
                break;
            }
            i += 1;
        }
        out
    };
    kthread_unlock();
    tid
}

pub fn kthread_wake(tid: u32) -> bool {
    if tid == 0 {
        return false;
    }
    kthread_lock();
    let ok = {
        let state = kthread_state_mut();
        if let Some(slot) = kthread_find_slot(state, tid) {
            state.entries[slot].runnable = 1;
            true
        } else {
            false
        }
    };
    kthread_unlock();
    ok
}

pub fn kthread_stop(tid: u32) -> bool {
    if tid == 0 {
        return false;
    }
    kthread_lock();
    let ok = {
        let state = kthread_state_mut();
        if let Some(slot) = kthread_find_slot(state, tid) {
            state.entries[slot].runnable = 0;
            true
        } else {
            false
        }
    };
    kthread_unlock();
    ok
}

pub fn kthread_count() -> u32 {
    kthread_lock();
    let count = {
        let state = kthread_state();
        let mut i = 0usize;
        let mut n = 0u32;
        while i < QOS_KTHREAD_MAX {
            if state.entries[i].used != 0 {
                n = n.saturating_add(1);
            }
            i += 1;
        }
        n
    };
    kthread_unlock();
    count
}

pub fn kthread_run_count(tid: u32) -> u32 {
    if tid == 0 {
        return 0;
    }
    kthread_lock();
    let count = {
        let state = kthread_state();
        match kthread_find_slot(state, tid) {
            Some(slot) => state.entries[slot].run_count,
            None => 0,
        }
    };
    kthread_unlock();
    count
}

pub fn kthread_run_next() -> u32 {
    let selected = {
        kthread_lock();
        let state = kthread_state_mut();
        let mut scanned = 0usize;
        let mut out = None;
        while scanned < QOS_KTHREAD_MAX {
            let idx = ((state.cursor as usize) + scanned) % QOS_KTHREAD_MAX;
            let entry = &mut state.entries[idx];
            if entry.used != 0 && entry.runnable != 0 {
                let fn_ptr = match entry.entry {
                    Some(v) => v,
                    None => {
                        scanned += 1;
                        continue;
                    }
                };
                entry.run_count = entry.run_count.saturating_add(1);
                state.cursor = ((idx + 1) % QOS_KTHREAD_MAX) as u32;
                state.current_tid = entry.tid;
                out = Some((entry.tid, fn_ptr, entry.arg));
                break;
            }
            scanned += 1;
        }
        if out.is_none() {
            state.current_tid = 0;
        }
        out
    };
    kthread_unlock();

    if let Some((tid, fn_ptr, arg)) = selected {
        fn_ptr(arg);
        tid
    } else {
        0
    }
}

pub fn kthread_current_tid() -> u32 {
    kthread_lock();
    let tid = kthread_state().current_tid;
    kthread_unlock();
    tid
}

pub fn kthread_name_get(tid: u32, out: &mut [u8]) -> Option<usize> {
    if out.is_empty() {
        return None;
    }
    kthread_lock();
    let copied = {
        let state = kthread_state();
        let slot = kthread_find_slot(state, tid)?;
        let len = state.name_len[slot] as usize;
        if len + 1 > out.len() {
            None
        } else {
            let mut i = 0usize;
            while i <= len {
                out[i] = state.names[slot][i];
                i += 1;
            }
            Some(len)
        }
    };
    kthread_unlock();
    copied
}

fn napi_find_slot(state: &NapiState, napi_id: u32) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_NAPI_MAX {
        let entry = &state.entries[i];
        if entry.used != 0 && entry.id == napi_id {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn napi_softirq_handler(_ctx: usize) {
    let mut need_reschedule = false;
    let mut i = 0usize;
    while i < QOS_NAPI_MAX {
        let maybe_poll = {
            napi_lock();
            let state = napi_state();
            let entry = &state.entries[i];
            if entry.used != 0 && entry.scheduled != 0 {
                let weight = if entry.weight == 0 { 1 } else { entry.weight };
                entry.poll.map(|poll| (poll, entry.ctx, weight))
            } else {
                None
            }
        };
        napi_unlock();

        if let Some((poll, ctx, weight)) = maybe_poll {
            let work = poll(ctx, weight);
            napi_lock();
            {
                let state = napi_state_mut();
                let entry = &mut state.entries[i];
                if entry.used != 0 {
                    entry.run_count = entry.run_count.saturating_add(1);
                    if work < weight {
                        entry.scheduled = 0;
                    } else {
                        need_reschedule = true;
                    }
                }
            }
            napi_unlock();
        }
        i += 1;
    }

    if need_reschedule {
        let _ = softirq_raise(QOS_SOFTIRQ_NET_RX);
    }
}

pub fn napi_reset() {
    napi_lock();
    *napi_state_mut() = NapiState::new();
    napi_unlock();
}

pub fn napi_register(weight: u32, poll: NapiPoll, ctx: usize) -> Option<u32> {
    if weight == 0 {
        return None;
    }
    napi_lock();
    let id = {
        let state = napi_state_mut();
        let mut i = 0usize;
        let mut out = None;
        while i < QOS_NAPI_MAX {
            if state.entries[i].used == 0 {
                let mut id = state.next_id;
                state.next_id = state.next_id.wrapping_add(1);
                if id == 0 {
                    id = state.next_id;
                    state.next_id = state.next_id.wrapping_add(1);
                }
                state.entries[i].used = 1;
                state.entries[i].scheduled = 0;
                state.entries[i].id = id;
                state.entries[i].weight = weight;
                state.entries[i].poll = Some(poll);
                state.entries[i].ctx = ctx;
                state.entries[i].run_count = 0;
                out = Some(id);
                break;
            }
            i += 1;
        }
        out
    };
    napi_unlock();
    id
}

pub fn napi_schedule(napi_id: u32) -> bool {
    if napi_id == 0 {
        return false;
    }
    let found = {
        napi_lock();
        let state = napi_state_mut();
        if let Some(slot) = napi_find_slot(state, napi_id) {
            state.entries[slot].scheduled = 1;
            true
        } else {
            false
        }
    };
    napi_unlock();
    if !found {
        return false;
    }
    softirq_raise(QOS_SOFTIRQ_NET_RX)
}

pub fn napi_complete(napi_id: u32) -> bool {
    if napi_id == 0 {
        return false;
    }
    napi_lock();
    let ok = {
        let state = napi_state_mut();
        if let Some(slot) = napi_find_slot(state, napi_id) {
            state.entries[slot].scheduled = 0;
            true
        } else {
            false
        }
    };
    napi_unlock();
    ok
}

pub fn napi_pending_count() -> u32 {
    napi_lock();
    let count = {
        let state = napi_state();
        let mut i = 0usize;
        let mut n = 0u32;
        while i < QOS_NAPI_MAX {
            if state.entries[i].used != 0 && state.entries[i].scheduled != 0 {
                n = n.saturating_add(1);
            }
            i += 1;
        }
        n
    };
    napi_unlock();
    count
}

pub fn napi_run_count(napi_id: u32) -> u32 {
    if napi_id == 0 {
        return 0;
    }
    napi_lock();
    let count = {
        let state = napi_state();
        match napi_find_slot(state, napi_id) {
            Some(slot) => state.entries[slot].run_count,
            None => 0,
        }
    };
    napi_unlock();
    count
}

fn workqueue_find_slot_by_id(state: &WorkqueueState, work_id: u32) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_WORKQUEUE_MAX {
        let entry = &state.entries[i];
        if entry.used != 0 && entry.id == work_id {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn workqueue_queue_push(state: &mut WorkqueueState, slot: u16) -> bool {
    if state.qcount as usize >= QOS_WORKQUEUE_MAX {
        return false;
    }
    state.queue[state.qtail as usize] = slot;
    state.qtail = ((state.qtail as usize + 1) % QOS_WORKQUEUE_MAX) as u16;
    state.qcount += 1;
    true
}

fn workqueue_queue_pop(state: &mut WorkqueueState) -> Option<u16> {
    if state.qcount == 0 {
        return None;
    }
    let slot = state.queue[state.qhead as usize];
    state.qhead = ((state.qhead as usize + 1) % QOS_WORKQUEUE_MAX) as u16;
    state.qcount -= 1;
    Some(slot)
}

fn workqueue_softirq_handler(_ctx: usize) {
    loop {
        let task = {
            workqueue_lock();
            let state = workqueue_state_mut();
            let slot = match workqueue_queue_pop(state) {
                Some(v) => v as usize,
                None => {
                    workqueue_unlock();
                    return;
                }
            };
            if slot >= QOS_WORKQUEUE_MAX {
                workqueue_unlock();
                continue;
            }
            let entry = &mut state.entries[slot];
            if entry.used == 0 || entry.pending == 0 {
                *entry = WorkqueueEntry::new();
                workqueue_unlock();
                continue;
            }
            let work = match entry.work {
                Some(v) => v,
                None => {
                    *entry = WorkqueueEntry::new();
                    workqueue_unlock();
                    continue;
                }
            };
            let arg = entry.arg;
            *entry = WorkqueueEntry::new();
            state.completed = state.completed.saturating_add(1);
            workqueue_unlock();
            (work, arg)
        };
        task.0(task.1);
    }
}

pub fn workqueue_reset() {
    workqueue_lock();
    *workqueue_state_mut() = WorkqueueState::new();
    workqueue_unlock();
}

pub fn workqueue_enqueue(work: WorkqueueFn, arg: usize) -> Option<u32> {
    let work_id = {
        workqueue_lock();
        let state = workqueue_state_mut();
        let mut i = 0usize;
        let mut out = None;
        while i < QOS_WORKQUEUE_MAX {
            if state.entries[i].used == 0 {
                let mut id = state.next_id;
                state.next_id = state.next_id.wrapping_add(1);
                if id == 0 {
                    id = state.next_id;
                    state.next_id = state.next_id.wrapping_add(1);
                }
                state.entries[i].used = 1;
                state.entries[i].pending = 1;
                state.entries[i].id = id;
                state.entries[i].work = Some(work);
                state.entries[i].arg = arg;
                if !workqueue_queue_push(state, i as u16) {
                    state.entries[i] = WorkqueueEntry::new();
                    out = None;
                } else {
                    out = Some(id);
                }
                break;
            }
            i += 1;
        }
        workqueue_unlock();
        out
    }?;
    let _ = softirq_raise(QOS_SOFTIRQ_WORKQUEUE);
    Some(work_id)
}

pub fn workqueue_cancel(work_id: u32) -> bool {
    if work_id == 0 {
        return false;
    }
    workqueue_lock();
    let ok = {
        let state = workqueue_state_mut();
        if let Some(slot) = workqueue_find_slot_by_id(state, work_id) {
            state.entries[slot] = WorkqueueEntry::new();
            true
        } else {
            false
        }
    };
    workqueue_unlock();
    ok
}

pub fn workqueue_pending_count() -> u32 {
    workqueue_lock();
    let count = {
        let state = workqueue_state();
        let mut i = 0usize;
        let mut n = 0u32;
        while i < QOS_WORKQUEUE_MAX {
            if state.entries[i].used != 0 && state.entries[i].pending != 0 {
                n = n.saturating_add(1);
            }
            i += 1;
        }
        n
    };
    workqueue_unlock();
    count
}

pub fn workqueue_completed_count() -> u32 {
    workqueue_lock();
    let count = workqueue_state().completed;
    workqueue_unlock();
    count
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct DriverNicDesc {
    pub mmio_base: u64,
    pub irq: u32,
    pub tx_desc_count: u16,
    pub rx_desc_count: u16,
    pub tx_head: u16,
    pub tx_tail: u16,
    pub rx_head: u16,
    pub rx_tail: u16,
}

fn driver_name_valid(name: &str) -> bool {
    let bytes = name.as_bytes();
    !bytes.is_empty() && bytes.len() < QOS_DRIVER_NAME_MAX
}

fn drivers_find(state: &DriversState, name: &str) -> Option<usize> {
    let bytes = name.as_bytes();
    let mut i = 0usize;
    while i < QOS_DRIVERS_MAX {
        if state.used[i] != 0
            && state.lens[i] as usize == bytes.len()
            && &state.names[i][..bytes.len()] == bytes
        {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn drivers_free_slot(state: &DriversState) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_DRIVERS_MAX {
        if state.used[i] == 0 {
            return Some(i);
        }
        i += 1;
    }
    None
}

pub fn drivers_reset() {
    drivers_lock();
    *drivers_state_mut() = DriversState::new();
    drivers_unlock();
}

pub fn drivers_register(name: &str) -> bool {
    if !driver_name_valid(name) {
        return false;
    }

    drivers_lock();
    let ok = (|| {
        let state = drivers_state_mut();
        if drivers_find(state, name).is_some() {
            return false;
        }
        let slot = match drivers_free_slot(state) {
            Some(s) => s,
            None => return false,
        };

        let bytes = name.as_bytes();
        let mut i = 0usize;
        while i < bytes.len() {
            state.names[slot][i] = bytes[i];
            i += 1;
        }
        state.lens[slot] = bytes.len() as u8;
        state.used[slot] = 1;
        state.nic_present[slot] = 0;
        state.count += 1;
        true
    })();
    drivers_unlock();
    ok
}

pub fn drivers_register_nic(
    name: &str,
    mmio_base: u64,
    irq: u32,
    tx_desc_count: u16,
    rx_desc_count: u16,
) -> bool {
    if !driver_name_valid(name) || tx_desc_count == 0 || rx_desc_count == 0 {
        return false;
    }

    drivers_lock();
    let ok = (|| {
        let state = drivers_state_mut();
        if drivers_find(state, name).is_some() {
            return false;
        }
        let slot = match drivers_free_slot(state) {
            Some(s) => s,
            None => return false,
        };

        let bytes = name.as_bytes();
        let mut i = 0usize;
        while i < bytes.len() {
            state.names[slot][i] = bytes[i];
            i += 1;
        }
        state.lens[slot] = bytes.len() as u8;
        state.used[slot] = 1;
        state.nic_present[slot] = 1;
        state.nic_mmio_base[slot] = mmio_base;
        state.nic_irq[slot] = irq;
        state.nic_tx_desc_count[slot] = tx_desc_count;
        state.nic_rx_desc_count[slot] = rx_desc_count;
        state.nic_tx_head[slot] = 0;
        state.nic_tx_tail[slot] = 0;
        state.nic_rx_head[slot] = 0;
        state.nic_rx_tail[slot] = 0;
        state.count += 1;
        true
    })();
    drivers_unlock();
    ok
}

pub fn drivers_loaded(name: &str) -> bool {
    if !driver_name_valid(name) {
        return false;
    }

    drivers_lock();
    let loaded = drivers_find(drivers_state(), name).is_some();
    drivers_unlock();
    loaded
}

pub fn drivers_count() -> u32 {
    drivers_lock();
    let count = drivers_state().count;
    drivers_unlock();
    count
}

pub fn drivers_get_nic(name: &str) -> Option<DriverNicDesc> {
    if !driver_name_valid(name) {
        return None;
    }
    drivers_lock();
    let nic = (|| {
        let state = drivers_state();
        let idx = drivers_find(state, name)?;
        if state.nic_present[idx] == 0 {
            return None;
        }
        Some(DriverNicDesc {
            mmio_base: state.nic_mmio_base[idx],
            irq: state.nic_irq[idx],
            tx_desc_count: state.nic_tx_desc_count[idx],
            rx_desc_count: state.nic_rx_desc_count[idx],
            tx_head: state.nic_tx_head[idx],
            tx_tail: state.nic_tx_tail[idx],
            rx_head: state.nic_rx_head[idx],
            rx_tail: state.nic_rx_tail[idx],
        })
    })();
    drivers_unlock();
    nic
}

pub fn drivers_nic_advance_tx(name: &str, produced: u16) -> bool {
    if !driver_name_valid(name) {
        return false;
    }
    drivers_lock();
    let ok = (|| {
        let state = drivers_state_mut();
        let idx = match drivers_find(state, name) {
            Some(v) => v,
            None => return false,
        };
        if state.nic_present[idx] == 0 || state.nic_tx_desc_count[idx] == 0 {
            return false;
        }
        let count = state.nic_tx_desc_count[idx] as u32;
        let next = (state.nic_tx_tail[idx] as u32 + produced as u32) % count;
        state.nic_tx_tail[idx] = next as u16;
        true
    })();
    drivers_unlock();
    ok
}

pub fn drivers_nic_consume_rx(name: &str, consumed: u16) -> bool {
    if !driver_name_valid(name) {
        return false;
    }
    drivers_lock();
    let ok = (|| {
        let state = drivers_state_mut();
        let idx = match drivers_find(state, name) {
            Some(v) => v,
            None => return false,
        };
        if state.nic_present[idx] == 0 || state.nic_rx_desc_count[idx] == 0 {
            return false;
        }
        let count = state.nic_rx_desc_count[idx] as u32;
        let next = (state.nic_rx_head[idx] as u32 + consumed as u32) % count;
        state.nic_rx_head[idx] = next as u16;
        true
    })();
    drivers_unlock();
    ok
}

pub fn syscall_reset() {
    syscall_lock();
    syscall_state_reset_in_place(syscall_state_mut());
    syscall_unlock();
}

pub fn syscall_register(nr: u32, op: u32, value: i64) -> bool {
    if nr as usize >= QOS_SYSCALL_MAX {
        return false;
    }
    if op != SYSCALL_OP_CONST
        && op != SYSCALL_OP_ADD
        && op != SYSCALL_OP_QUERY
        && op != SYSCALL_OP_KILL
        && op != SYSCALL_OP_SIGPENDING
        && op != SYSCALL_OP_SIGACTION
        && op != SYSCALL_OP_SIGPROCMASK
        && op != SYSCALL_OP_SIGALTSTACK
        && op != SYSCALL_OP_SIGSUSPEND
        && op != SYSCALL_OP_FORK
        && op != SYSCALL_OP_EXEC
        && op != SYSCALL_OP_SIGRETURN
        && op != SYSCALL_OP_EXIT
        && op != SYSCALL_OP_GETPID
        && op != SYSCALL_OP_WAITPID
        && op != SYSCALL_OP_STAT
        && op != SYSCALL_OP_MKDIR
        && op != SYSCALL_OP_UNLINK
        && op != SYSCALL_OP_CHDIR
        && op != SYSCALL_OP_GETCWD
        && op != SYSCALL_OP_OPEN
        && op != SYSCALL_OP_READ
        && op != SYSCALL_OP_WRITE
        && op != SYSCALL_OP_CLOSE
        && op != SYSCALL_OP_DUP2
        && op != SYSCALL_OP_MMAP
        && op != SYSCALL_OP_MUNMAP
        && op != SYSCALL_OP_YIELD
        && op != SYSCALL_OP_SLEEP
        && op != SYSCALL_OP_SOCKET
        && op != SYSCALL_OP_BIND
        && op != SYSCALL_OP_LISTEN
        && op != SYSCALL_OP_ACCEPT
        && op != SYSCALL_OP_CONNECT
        && op != SYSCALL_OP_SEND
        && op != SYSCALL_OP_RECV
        && op != SYSCALL_OP_GETDENTS
        && op != SYSCALL_OP_LSEEK
        && op != SYSCALL_OP_PIPE
        && op != SYSCALL_OP_DLOPEN
        && op != SYSCALL_OP_DLCLOSE
        && op != SYSCALL_OP_DLSYM
        && op != SYSCALL_OP_MODLOAD
        && op != SYSCALL_OP_MODUNLOAD
        && op != SYSCALL_OP_MODRELOAD
    {
        return false;
    }

    syscall_lock();
    let ok = (|| {
        let state = syscall_state_mut();
        let idx = nr as usize;
        if state.used[idx] != 0 {
            return false;
        }

        state.used[idx] = 1;
        state.ops[idx] = op as u8;
        state.values[idx] = value;
        state.count += 1;
        true
    })();
    syscall_unlock();
    ok
}

pub fn syscall_unregister(nr: u32) -> bool {
    if nr as usize >= QOS_SYSCALL_MAX {
        return false;
    }

    syscall_lock();
    let ok = (|| {
        let state = syscall_state_mut();
        let idx = nr as usize;
        if state.used[idx] == 0 {
            return false;
        }

        state.used[idx] = 0;
        state.ops[idx] = 0;
        state.values[idx] = 0;
        state.count -= 1;
        true
    })();
    syscall_unlock();
    ok
}

fn syscall_fd_valid(state: &SyscallState, fd: u32) -> bool {
    (fd as usize) < QOS_FD_MAX && state.fd_used[fd as usize] != 0
}

fn syscall_sock_port_table_mut(state: &mut SyscallState, sock_type: u8) -> Option<&mut [u32; QOS_SOCK_PORT_WORDS]> {
    match sock_type {
        SOCK_STREAM => Some(&mut state.sock_port_stream),
        SOCK_DGRAM => Some(&mut state.sock_port_dgram),
        _ => None,
    }
}

fn syscall_sock_port_index(port: u16) -> Option<(usize, u32)> {
    if !(QOS_SOCK_PORT_MIN..=QOS_SOCK_PORT_MAX).contains(&port) {
        return None;
    }
    let idx = port as usize - QOS_SOCK_PORT_MIN as usize;
    Some((idx / 32, 1u32 << (idx % 32)))
}

fn syscall_sock_port_mark(state: &mut SyscallState, sock_type: u8, port: u16) -> bool {
    let (word, mask) = match syscall_sock_port_index(port) {
        Some(v) => v,
        None => return false,
    };
    let table = match syscall_sock_port_table_mut(state, sock_type) {
        Some(v) => v,
        None => return false,
    };
    if (table[word] & mask) != 0 {
        return false;
    }
    table[word] |= mask;
    true
}

fn syscall_sock_port_unmark(state: &mut SyscallState, sock_type: u8, port: u16) {
    let (word, mask) = match syscall_sock_port_index(port) {
        Some(v) => v,
        None => return,
    };
    let table = match syscall_sock_port_table_mut(state, sock_type) {
        Some(v) => v,
        None => return,
    };
    table[word] &= !mask;
}

fn syscall_sock_port_alloc(state: &mut SyscallState, sock_type: u8) -> Option<u16> {
    let mut port = 49152u32;
    while port <= QOS_SOCK_PORT_MAX as u32 {
        let p = port as u16;
        if syscall_sock_port_mark(state, sock_type, p) {
            return Some(p);
        }
        port += 1;
    }
    port = QOS_SOCK_PORT_MIN as u32;
    while port < 49152u32 {
        let p = port as u16;
        if syscall_sock_port_mark(state, sock_type, p) {
            return Some(p);
        }
        port += 1;
    }
    None
}

fn syscall_sock_port_in_use(state: &SyscallState, port: u16, sock_type: u8) -> bool {
    let mut i = 0usize;
    while i < QOS_FD_MAX {
        if state.fd_used[i] != 0
            && state.fd_kind[i] == FD_KIND_SOCKET
            && state.fd_sock_bound[i] != 0
            && state.fd_sock_type[i] == sock_type
            && state.fd_sock_lport[i] == port
        {
            return true;
        }
        i += 1;
    }
    false
}

fn syscall_sock_listener_in_use(state: &SyscallState, port: u16) -> bool {
    let mut i = 0usize;
    while i < QOS_FD_MAX {
        if state.fd_used[i] != 0
            && state.fd_kind[i] == FD_KIND_SOCKET
            && state.fd_sock_type[i] == SOCK_STREAM
            && state.fd_sock_listening[i] != 0
            && state.fd_sock_lport[i] == port
        {
            return true;
        }
        i += 1;
    }
    false
}

fn syscall_sock_pending_push(state: &mut SyscallState, fd: u32, peer_port: u16, peer_ip: u32, conn_id: i16) -> bool {
    let idx = fd as usize;
    if idx >= QOS_FD_MAX || state.fd_sock_pending[idx] as usize >= QOS_SOCK_PENDING_MAX {
        return false;
    }
    let tail = state.fd_sock_pending_tail[idx] as usize;
    state.fd_sock_pending_q_rport[idx][tail] = peer_port;
    state.fd_sock_pending_q_rip[idx][tail] = peer_ip;
    state.fd_sock_pending_q_conn_id[idx][tail] = conn_id;
    state.fd_sock_pending_tail[idx] = ((tail + 1) % QOS_SOCK_PENDING_MAX) as u8;
    state.fd_sock_pending[idx] = state.fd_sock_pending[idx].saturating_add(1);
    true
}

fn syscall_sock_pending_pop(state: &mut SyscallState, fd: u32) -> Option<(u16, u32, i16)> {
    let idx = fd as usize;
    if idx >= QOS_FD_MAX || state.fd_sock_pending[idx] == 0 {
        return None;
    }
    let head = state.fd_sock_pending_head[idx] as usize;
    let peer_port = state.fd_sock_pending_q_rport[idx][head];
    let peer_ip = state.fd_sock_pending_q_rip[idx][head];
    let conn_id = state.fd_sock_pending_q_conn_id[idx][head];
    state.fd_sock_pending_q_rport[idx][head] = 0;
    state.fd_sock_pending_q_rip[idx][head] = 0;
    state.fd_sock_pending_q_conn_id[idx][head] = -1;
    state.fd_sock_pending_head[idx] = ((head + 1) % QOS_SOCK_PENDING_MAX) as u8;
    state.fd_sock_pending[idx] -= 1;
    if state.fd_sock_pending[idx] == 0 {
        state.fd_sock_pending_head[idx] = 0;
        state.fd_sock_pending_tail[idx] = 0;
    }
    Some((peer_port, peer_ip, conn_id))
}

fn syscall_fd_alloc(state: &mut SyscallState, kind: u8) -> Option<u32> {
    let mut i = 0usize;
    while i < QOS_FD_MAX {
        if state.fd_used[i] == 0 {
            state.fd_used[i] = 1;
            state.fd_kind[i] = kind;
            state.fd_sock_bound[i] = 0;
            state.fd_sock_listening[i] = 0;
            state.fd_sock_connected[i] = 0;
            state.fd_sock_pending[i] = 0;
            state.fd_sock_backlog[i] = 0;
            state.fd_sock_type[i] = 0;
            state.fd_sock_lport[i] = 0;
            state.fd_sock_rport[i] = 0;
            state.fd_sock_rip[i] = 0;
            state.fd_sock_conn_id[i] = -1;
            state.fd_sock_stream_len[i] = 0;
            state.fd_sock_stream_buf[i] = [0; QOS_NET_MAX_PACKET];
            state.fd_sock_raw_len[i] = 0;
            state.fd_sock_raw_src_ip[i] = 0;
            state.fd_sock_raw_buf[i] = [0; QOS_NET_MAX_PACKET];
            state.fd_sock_pending_rport[i] = 0;
            state.fd_sock_pending_rip[i] = 0;
            state.fd_sock_pending_conn_id[i] = -1;
            state.fd_sock_pending_head[i] = 0;
            state.fd_sock_pending_tail[i] = 0;
            state.fd_sock_pending_q_rport[i] = [0; QOS_SOCK_PENDING_MAX];
            state.fd_sock_pending_q_rip[i] = [0; QOS_SOCK_PENDING_MAX];
            state.fd_sock_pending_q_conn_id[i] = [-1; QOS_SOCK_PENDING_MAX];
            state.fd_pipe_id[i] = 0;
            state.fd_offset[i] = 0;
            state.fd_file_id[i] = u16::MAX;
            state.fd_proc_kind[i] = PROC_FD_NONE;
            state.fd_proc_pid[i] = 0;
            return Some(i as u32);
        }
        i += 1;
    }
    None
}

fn syscall_pipe_in_use(state: &SyscallState, pipe_id: u16) -> bool {
    let mut i = 0usize;
    while i < QOS_FD_MAX {
        if state.fd_used[i] != 0
            && (state.fd_kind[i] == FD_KIND_PIPE_R || state.fd_kind[i] == FD_KIND_PIPE_W)
            && state.fd_pipe_id[i] == pipe_id
        {
            return true;
        }
        i += 1;
    }
    false
}

fn syscall_pipe_alloc(state: &mut SyscallState) -> Option<u16> {
    let mut i = 0usize;
    while i < QOS_PIPE_MAX {
        if state.pipe_used[i] == 0 {
            state.pipe_used[i] = 1;
            state.pipe_len[i] = 0;
            state.pipe_buf[i] = [0; QOS_PIPE_CAP];
            return Some(i as u16);
        }
        i += 1;
    }
    None
}

fn syscall_file_find(state: &SyscallState, path: &str) -> Option<usize> {
    let bytes = path.as_bytes();
    let mut i = 0usize;
    while i < QOS_VFS_MAX_NODES {
        if state.file_used[i] != 0
            && state.file_path_len[i] as usize == bytes.len()
            && &state.file_paths[i][..bytes.len()] == bytes
        {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn syscall_file_alloc(state: &mut SyscallState, path: &str) -> Option<usize> {
    let bytes = path.as_bytes();
    let mut i = 0usize;
    while i < QOS_VFS_MAX_NODES {
        if state.file_used[i] == 0 {
            state.file_used[i] = 1;
            state.file_paths[i] = [0; QOS_VFS_PATH_MAX];
            state.file_paths[i][..bytes.len()].copy_from_slice(bytes);
            state.file_path_len[i] = bytes.len() as u8;
            state.file_len[i] = 0;
            state.file_data[i] = [0; QOS_PIPE_CAP];
            return Some(i);
        }
        i += 1;
    }
    None
}

fn syscall_file_ensure(state: &mut SyscallState, path: &str) -> Option<usize> {
    match syscall_file_find(state, path) {
        Some(i) => Some(i),
        None => syscall_file_alloc(state, path),
    }
}

fn syscall_file_has_refs(state: &SyscallState, file_id: usize) -> bool {
    if file_id >= QOS_VFS_MAX_NODES {
        return false;
    }
    let fid = file_id as u16;
    let mut i = 0usize;
    while i < QOS_FD_MAX {
        if state.fd_used[i] != 0 && state.fd_kind[i] == FD_KIND_VFS && state.fd_file_id[i] == fid {
            return true;
        }
        i += 1;
    }
    false
}

fn syscall_file_drop(state: &mut SyscallState, file_id: usize) {
    if file_id >= QOS_VFS_MAX_NODES {
        return;
    }
    state.file_used[file_id] = 0;
    state.file_paths[file_id] = [0; QOS_VFS_PATH_MAX];
    state.file_path_len[file_id] = 0;
    state.file_len[file_id] = 0;
    state.file_data[file_id] = [0; QOS_PIPE_CAP];
}

fn syscall_file_maybe_gc(state: &mut SyscallState, file_id: usize) {
    if file_id >= QOS_VFS_MAX_NODES
        || state.file_used[file_id] == 0
        || state.file_path_len[file_id] != 0
        || syscall_file_has_refs(state, file_id)
    {
        return;
    }
    syscall_file_drop(state, file_id);
}

fn elf_u16(image: &[u8], off: usize) -> Option<u16> {
    if off + 2 > image.len() {
        return None;
    }
    Some((image[off] as u16) | ((image[off + 1] as u16) << 8))
}

fn elf_u32(image: &[u8], off: usize) -> Option<u32> {
    if off + 4 > image.len() {
        return None;
    }
    Some(
        (image[off] as u32)
            | ((image[off + 1] as u32) << 8)
            | ((image[off + 2] as u32) << 16)
            | ((image[off + 3] as u32) << 24),
    )
}

fn elf_u64(image: &[u8], off: usize) -> Option<u64> {
    if off + 8 > image.len() {
        return None;
    }
    Some(
        (image[off] as u64)
            | ((image[off + 1] as u64) << 8)
            | ((image[off + 2] as u64) << 16)
            | ((image[off + 3] as u64) << 24)
            | ((image[off + 4] as u64) << 32)
            | ((image[off + 5] as u64) << 40)
            | ((image[off + 6] as u64) << 48)
            | ((image[off + 7] as u64) << 56),
    )
}

fn parse_exec_elf_image(image: &[u8]) -> Option<ProcExecImage> {
    if image.len() < 64 {
        return None;
    }
    if image[0] != 0x7F || image[1] != b'E' || image[2] != b'L' || image[3] != b'F' {
        return None;
    }
    if image[4] != ELF_CLASS_64 || image[5] != ELF_DATA_LSB || image[6] != 1 {
        return None;
    }

    let e_type = elf_u16(image, 16)?;
    let e_machine = elf_u16(image, 18)?;
    let e_entry = elf_u64(image, 24)?;
    let e_phoff = elf_u64(image, 32)?;
    let e_phentsize = elf_u16(image, 54)?;
    let e_phnum = elf_u16(image, 56)?;
    if (e_type != ELF_ET_EXEC && e_type != ELF_ET_DYN)
        || (e_machine != ELF_EM_X86_64 && e_machine != ELF_EM_AARCH64)
    {
        return None;
    }
    if e_phnum == 0 || e_phentsize < 56 {
        return None;
    }
    let phoff = e_phoff as usize;
    let phentsize = e_phentsize as usize;
    let phnum = e_phnum as usize;
    if phoff > image.len() {
        return None;
    }
    if phnum > (usize::MAX - phoff) / phentsize {
        return None;
    }
    if phoff + phnum * phentsize > image.len() {
        return None;
    }

    let mut load_count = 0u16;
    let mut has_interp = false;
    let mut interp_off = 0u64;
    let mut interp_len = 0u64;
    let mut i = 0usize;
    while i < phnum {
        let p_off = phoff + i * phentsize;
        let p_type = elf_u32(image, p_off)?;
        let p_offset = elf_u64(image, p_off + 8)?;
        let p_vaddr = elf_u64(image, p_off + 16)?;
        let p_filesz = elf_u64(image, p_off + 32)?;
        let p_memsz = elf_u64(image, p_off + 40)?;
        let p_align = elf_u64(image, p_off + 48)?;

        if p_type == ELF_PT_LOAD {
            if p_memsz < p_filesz {
                return None;
            }
            if p_filesz != 0 {
                let off = p_offset as usize;
                let sz = p_filesz as usize;
                if off > image.len() || sz > image.len() - off {
                    return None;
                }
            }
            if p_align != 0 && (p_vaddr % p_align) != (p_offset % p_align) {
                return None;
            }
            load_count = load_count.saturating_add(1);
        } else if p_type == ELF_PT_INTERP {
            let off = p_offset as usize;
            let sz = p_filesz as usize;
            if sz == 0 || off > image.len() || sz > image.len() - off {
                return None;
            }
            if image[off + sz - 1] != 0 {
                return None;
            }
            has_interp = true;
            interp_off = p_offset;
            interp_len = p_filesz;
        }
        i += 1;
    }

    if load_count == 0 {
        return None;
    }

    Some(ProcExecImage {
        entry: e_entry,
        phoff: e_phoff,
        phentsize: e_phentsize,
        phnum: e_phnum,
        load_count,
        has_interp,
        interp_off,
        interp_len,
    })
}

fn parse_shared_elf_image(image: &[u8]) -> bool {
    if image.len() < 64 {
        return false;
    }
    if elf_u16(image, 16) != Some(ELF_ET_DYN) {
        return false;
    }
    parse_exec_elf_image(image).is_some()
}

fn syscall_plugin_path_valid(path: &str) -> bool {
    let bytes = path.as_bytes();
    !bytes.is_empty() && bytes[0] == b'/' && bytes.len() < QOS_VFS_PATH_MAX
}

fn syscall_copy_path(dst: &mut [u8; QOS_VFS_PATH_MAX], dst_len: &mut u8, path: &str) -> bool {
    if !syscall_plugin_path_valid(path) {
        return false;
    }
    let bytes = path.as_bytes();
    *dst = [0; QOS_VFS_PATH_MAX];
    dst[..bytes.len()].copy_from_slice(bytes);
    *dst_len = bytes.len() as u8;
    true
}

fn syscall_shlib_find_handle(state: &SyscallState, handle: u32) -> Option<usize> {
    if handle == 0 {
        return None;
    }
    let mut i = 0usize;
    while i < QOS_SHLIB_MAX {
        if state.shlib_used[i] != 0 && state.shlib_handle[i] == handle {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn syscall_shlib_find_path(state: &SyscallState, path: &str) -> Option<usize> {
    let bytes = path.as_bytes();
    let mut i = 0usize;
    while i < QOS_SHLIB_MAX {
        if state.shlib_used[i] != 0
            && state.shlib_path_len[i] as usize == bytes.len()
            && &state.shlib_paths[i][..bytes.len()] == bytes
        {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn syscall_shlib_free_slot(state: &SyscallState) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_SHLIB_MAX {
        if state.shlib_used[i] == 0 {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn syscall_shlib_next_handle(state: &mut SyscallState) -> u32 {
    let mut handle = state.shlib_next_handle;
    state.shlib_next_handle = state.shlib_next_handle.wrapping_add(1);
    if handle == 0 {
        handle = state.shlib_next_handle;
        state.shlib_next_handle = state.shlib_next_handle.wrapping_add(1);
    }
    if handle == 0 {
        handle = 1;
        state.shlib_next_handle = 2;
    }
    handle
}

fn syscall_shlib_load(state: &mut SyscallState, path: &str, force_new: bool) -> Option<u32> {
    if !syscall_plugin_path_valid(path) {
        return None;
    }
    if !force_new {
        if let Some(slot) = syscall_shlib_find_path(state, path) {
            state.shlib_refcount[slot] = state.shlib_refcount[slot].saturating_add(1);
            return Some(state.shlib_handle[slot]);
        }
    }

    let file_id = syscall_file_find(state, path)?;
    if state.file_used[file_id] == 0 {
        return None;
    }
    let len = state.file_len[file_id] as usize;
    if len == 0 || !parse_shared_elf_image(&state.file_data[file_id][..len]) {
        return None;
    }

    let slot = syscall_shlib_free_slot(state)?;
    let handle = syscall_shlib_next_handle(state);
    state.shlib_used[slot] = 1;
    state.shlib_handle[slot] = handle;
    state.shlib_refcount[slot] = 1;
    state.shlib_file_id[slot] = file_id as u16;
    if !syscall_copy_path(&mut state.shlib_paths[slot], &mut state.shlib_path_len[slot], path) {
        state.shlib_used[slot] = 0;
        state.shlib_handle[slot] = 0;
        state.shlib_refcount[slot] = 0;
        state.shlib_file_id[slot] = u16::MAX;
        state.shlib_paths[slot] = [0; QOS_VFS_PATH_MAX];
        state.shlib_path_len[slot] = 0;
        return None;
    }
    Some(handle)
}

fn syscall_shlib_release(state: &mut SyscallState, handle: u32) -> bool {
    let slot = match syscall_shlib_find_handle(state, handle) {
        Some(v) => v,
        None => return false,
    };
    if state.shlib_refcount[slot] == 0 {
        return false;
    }
    state.shlib_refcount[slot] -= 1;
    if state.shlib_refcount[slot] == 0 {
        state.shlib_used[slot] = 0;
        state.shlib_handle[slot] = 0;
        state.shlib_refcount[slot] = 0;
        state.shlib_file_id[slot] = u16::MAX;
        state.shlib_paths[slot] = [0; QOS_VFS_PATH_MAX];
        state.shlib_path_len[slot] = 0;
    }
    true
}

fn syscall_module_find_id(state: &SyscallState, module_id: u32) -> Option<usize> {
    if module_id == 0 {
        return None;
    }
    let mut i = 0usize;
    while i < QOS_MODULE_MAX {
        if state.module_used[i] != 0 && state.module_id[i] == module_id {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn syscall_module_find_path(state: &SyscallState, path: &str) -> Option<usize> {
    let bytes = path.as_bytes();
    let mut i = 0usize;
    while i < QOS_MODULE_MAX {
        if state.module_used[i] != 0
            && state.module_path_len[i] as usize == bytes.len()
            && &state.module_paths[i][..bytes.len()] == bytes
        {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn syscall_module_free_slot(state: &SyscallState) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_MODULE_MAX {
        if state.module_used[i] == 0 {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn syscall_module_next_id(state: &mut SyscallState) -> u32 {
    let mut id = state.module_next_id;
    state.module_next_id = state.module_next_id.wrapping_add(1);
    if id == 0 {
        id = state.module_next_id;
        state.module_next_id = state.module_next_id.wrapping_add(1);
    }
    if id == 0 {
        id = 1;
        state.module_next_id = 2;
    }
    id
}

fn syscall_module_load(state: &mut SyscallState, path: &str) -> Option<u32> {
    if !syscall_plugin_path_valid(path) {
        return None;
    }
    if let Some(slot) = syscall_module_find_path(state, path) {
        return Some(state.module_id[slot]);
    }
    let handle = syscall_shlib_load(state, path, false)?;
    let slot = match syscall_module_free_slot(state) {
        Some(v) => v,
        None => {
            let _ = syscall_shlib_release(state, handle);
            return None;
        }
    };
    let module_id = syscall_module_next_id(state);
    state.module_used[slot] = 1;
    state.module_id[slot] = module_id;
    state.module_generation[slot] = 1;
    state.module_shlib_handle[slot] = handle;
    if !syscall_copy_path(&mut state.module_paths[slot], &mut state.module_path_len[slot], path) {
        state.module_used[slot] = 0;
        state.module_id[slot] = 0;
        state.module_generation[slot] = 0;
        state.module_shlib_handle[slot] = 0;
        let _ = syscall_shlib_release(state, handle);
        return None;
    }
    Some(module_id)
}

fn syscall_module_unload(state: &mut SyscallState, module_id: u32) -> bool {
    let slot = match syscall_module_find_id(state, module_id) {
        Some(v) => v,
        None => return false,
    };
    let handle = state.module_shlib_handle[slot];
    state.module_used[slot] = 0;
    state.module_id[slot] = 0;
    state.module_generation[slot] = 0;
    state.module_shlib_handle[slot] = 0;
    state.module_paths[slot] = [0; QOS_VFS_PATH_MAX];
    state.module_path_len[slot] = 0;
    if handle != 0 {
        let _ = syscall_shlib_release(state, handle);
    }
    true
}

fn syscall_module_reload(state: &mut SyscallState, module_id: u32) -> Option<u32> {
    let slot = syscall_module_find_id(state, module_id)?;
    let mut path_buf = [0u8; QOS_VFS_PATH_MAX];
    let path_len = state.module_path_len[slot] as usize;
    if path_len == 0 || path_len >= QOS_VFS_PATH_MAX {
        return None;
    }
    path_buf[..path_len].copy_from_slice(&state.module_paths[slot][..path_len]);
    let path = core::str::from_utf8(&path_buf[..path_len]).ok()?;

    let new_handle = syscall_shlib_load(state, path, true)?;
    let old_handle = state.module_shlib_handle[slot];
    state.module_shlib_handle[slot] = new_handle;
    state.module_generation[slot] = state.module_generation[slot].saturating_add(1);
    if state.module_generation[slot] == 0 {
        state.module_generation[slot] = 1;
    }
    if old_handle != 0 {
        let _ = syscall_shlib_release(state, old_handle);
    }
    Some(state.module_generation[slot])
}

fn symbol_hash32(name: &str) -> u32 {
    let bytes = name.as_bytes();
    if bytes.is_empty() {
        return 0;
    }
    let mut hash = 5381u32;
    let mut i = 0usize;
    while i < bytes.len() {
        hash = ((hash << 5).wrapping_add(hash)) ^ bytes[i] as u32;
        i += 1;
    }
    hash
}

fn syscall_fd_clear(state: &mut SyscallState, fd: u32) {
    if !syscall_fd_valid(state, fd) {
        return;
    }
    let idx = fd as usize;
    let pipe_id = state.fd_pipe_id[idx];
    let file_id = state.fd_file_id[idx] as usize;
    let was_socket = state.fd_kind[idx] == FD_KIND_SOCKET;
    let sock_type = state.fd_sock_type[idx];
    let sock_listening = state.fd_sock_listening[idx] != 0;
    let sock_bound = state.fd_sock_bound[idx] != 0;
    let sock_lport = state.fd_sock_lport[idx];
    state.fd_used[idx] = 0;
    state.fd_kind[idx] = FD_KIND_NONE;
    state.fd_sock_bound[idx] = 0;
    state.fd_sock_listening[idx] = 0;
    state.fd_sock_connected[idx] = 0;
    state.fd_sock_pending[idx] = 0;
    state.fd_sock_backlog[idx] = 0;
    state.fd_sock_type[idx] = 0;
    state.fd_sock_lport[idx] = 0;
    state.fd_sock_rport[idx] = 0;
    state.fd_sock_rip[idx] = 0;
    state.fd_sock_conn_id[idx] = -1;
    state.fd_sock_stream_len[idx] = 0;
    state.fd_sock_stream_buf[idx] = [0; QOS_NET_MAX_PACKET];
    state.fd_sock_raw_len[idx] = 0;
    state.fd_sock_raw_src_ip[idx] = 0;
    state.fd_sock_raw_buf[idx] = [0; QOS_NET_MAX_PACKET];
    state.fd_sock_pending_rport[idx] = 0;
    state.fd_sock_pending_rip[idx] = 0;
    state.fd_sock_pending_conn_id[idx] = -1;
    state.fd_sock_pending_head[idx] = 0;
    state.fd_sock_pending_tail[idx] = 0;
    state.fd_sock_pending_q_rport[idx] = [0; QOS_SOCK_PENDING_MAX];
    state.fd_sock_pending_q_rip[idx] = [0; QOS_SOCK_PENDING_MAX];
    state.fd_sock_pending_q_conn_id[idx] = [-1; QOS_SOCK_PENDING_MAX];
    state.fd_pipe_id[idx] = 0;
    state.fd_offset[idx] = 0;
    state.fd_file_id[idx] = u16::MAX;
    state.fd_proc_kind[idx] = PROC_FD_NONE;
    state.fd_proc_pid[idx] = 0;

    if (pipe_id as usize) < QOS_PIPE_MAX && !syscall_pipe_in_use(state, pipe_id) {
        let pidx = pipe_id as usize;
        state.pipe_used[pidx] = 0;
        state.pipe_len[pidx] = 0;
        state.pipe_buf[pidx] = [0; QOS_PIPE_CAP];
    }

    syscall_file_maybe_gc(state, file_id);

    if was_socket && sock_type == SOCK_STREAM && sock_listening && !syscall_sock_listener_in_use(state, sock_lport) {
        let _ = net_tcp_unlisten(sock_lport);
    }

    if was_socket && sock_bound && !syscall_sock_port_in_use(state, sock_lport, sock_type) {
        if sock_type == SOCK_DGRAM {
            let _ = net_udp_unbind(sock_lport);
        }
        syscall_sock_port_unmark(state, sock_type, sock_lport);
    }
}

fn syscall_parse_sockaddr(addr: u64, len: u64) -> Option<(u16, u32)> {
    if addr == 0 || len < 8 {
        return None;
    }
    let bytes = unsafe { core::slice::from_raw_parts(addr as *const u8, len as usize) };
    if bytes.len() < 8 {
        return None;
    }
    let port = ((bytes[2] as u16) << 8) | bytes[3] as u16;
    let ip = ((bytes[4] as u32) << 24)
        | ((bytes[5] as u32) << 16)
        | ((bytes[6] as u32) << 8)
        | (bytes[7] as u32);
    if port == 0 || ip == 0 {
        return None;
    }
    Some((port, ip))
}

fn syscall_write_sockaddr(addr: u64, port: u16, ip: u32) {
    if addr == 0 {
        return;
    }
    unsafe {
        let out = core::slice::from_raw_parts_mut(addr as *mut u8, 16);
        out.fill(0);
        out[0] = 2;
        out[1] = 0;
        out[2] = (port >> 8) as u8;
        out[3] = (port & 0xFF) as u8;
        out[4] = (ip >> 24) as u8;
        out[5] = ((ip >> 16) & 0xFF) as u8;
        out[6] = ((ip >> 8) & 0xFF) as u8;
        out[7] = (ip & 0xFF) as u8;
    }
}

fn syscall_stream_host_response_prepare(state: &mut SyscallState, fd: usize, req: &[u8]) -> bool {
    const GET_PREFIX: &[u8] = b"GET ";
    const RESP: &[u8] = b"HTTP/1.0 200 OK\r\nContent-Length: 4\r\n\r\nqos\n";
    if fd >= QOS_FD_MAX || req.len() < GET_PREFIX.len() || &req[..GET_PREFIX.len()] != GET_PREFIX || RESP.len() > QOS_NET_MAX_PACKET {
        return false;
    }
    state.fd_sock_stream_buf[fd] = [0; QOS_NET_MAX_PACKET];
    state.fd_sock_stream_buf[fd][..RESP.len()].copy_from_slice(RESP);
    state.fd_sock_stream_len[fd] = RESP.len() as u16;
    true
}

fn syscall_proc_path_kind(path: &str) -> Option<(u8, u32)> {
    if path == "/proc/meminfo" {
        return Some((PROC_FD_MEMINFO, 0));
    }
    if path == "/proc/kernel/status" {
        return Some((PROC_FD_KERNEL_STATUS, 0));
    }
    if path == "/proc/net/dev" {
        return Some((PROC_FD_NET_DEV, 0));
    }
    if path == "/proc/syscalls" {
        return Some((PROC_FD_SYSCALLS, 0));
    }
    if path == "/proc/uptime" {
        return Some((PROC_FD_UPTIME, 0));
    }
    if path == "/proc/runtime/map" {
        return Some((PROC_FD_RUNTIME_MAP, 0));
    }
    if !path.starts_with("/proc/") || !path.ends_with("/status") {
        return None;
    }
    let pid_part = &path[6..path.len().saturating_sub(7)];
    if pid_part.is_empty() {
        return None;
    }
    let mut pid = 0u32;
    for &b in pid_part.as_bytes() {
        if !b.is_ascii_digit() {
            return None;
        }
        pid = pid.saturating_mul(10).saturating_add((b - b'0') as u32);
    }
    if pid == 0 {
        return None;
    }
    Some((PROC_FD_STATUS, pid))
}

fn append_bytes(out: &mut [u8], cursor: &mut usize, src: &[u8]) {
    let start = *cursor;
    *cursor = (*cursor).saturating_add(src.len());
    if start >= out.len() {
        return;
    }
    let copy_len = core::cmp::min(src.len(), out.len() - start);
    out[start..start + copy_len].copy_from_slice(&src[..copy_len]);
}

fn append_u32_ascii(out: &mut [u8], cursor: &mut usize, value: u32) {
    let mut buf = [0u8; 10];
    let mut n = 0usize;
    let mut v = value;
    if v == 0 {
        append_bytes(out, cursor, b"0");
        return;
    }
    while v > 0 {
        buf[n] = b'0' + (v % 10) as u8;
        v /= 10;
        n += 1;
    }
    while n > 0 {
        n -= 1;
        append_bytes(out, cursor, &buf[n..n + 1]);
    }
}

fn append_u32_hex(out: &mut [u8], cursor: &mut usize, mut value: u32) {
    const HEX: &[u8; 16] = b"0123456789abcdef";
    if value == 0 {
        append_bytes(out, cursor, b"0");
        return;
    }
    let mut tmp = [0u8; 8];
    let mut n = 0usize;
    while value != 0 && n < tmp.len() {
        tmp[n] = HEX[(value & 0xF) as usize];
        value >>= 4;
        n += 1;
    }
    while n > 0 {
        n -= 1;
        append_bytes(out, cursor, &tmp[n..n + 1]);
    }
}

fn append_u64_hex_fixed(out: &mut [u8], cursor: &mut usize, value: u64, digits: usize) {
    const HEX: &[u8; 16] = b"0123456789abcdef";
    let digits = core::cmp::min(digits, 16);
    let mut i = 0usize;
    while i < digits {
        let shift = ((digits - 1 - i) * 4) as u32;
        let nibble = ((value >> shift) & 0xF) as usize;
        append_bytes(out, cursor, &HEX[nibble..nibble + 1]);
        i += 1;
    }
}

fn syscall_proc_render(state: &SyscallState, fd_idx: usize, out: &mut [u8]) -> usize {
    if fd_idx >= QOS_FD_MAX || state.fd_kind[fd_idx] != FD_KIND_PROC {
        return 0;
    }

    let mut total = 0usize;
    match state.fd_proc_kind[fd_idx] {
        PROC_FD_MEMINFO => {
            let total_pages = pmm_total_pages();
            let free_pages = pmm_free_pages();
            let proc_total = proc_count();
            append_bytes(out, &mut total, b"MemTotal:\t");
            append_u32_ascii(out, &mut total, total_pages.saturating_mul(4));
            append_bytes(out, &mut total, b" kB\nMemFree:\t");
            append_u32_ascii(out, &mut total, free_pages.saturating_mul(4));
            append_bytes(out, &mut total, b" kB\nProcCount:\t");
            append_u32_ascii(out, &mut total, proc_total);
            append_bytes(out, &mut total, b"\n");
        }
        PROC_FD_KERNEL_STATUS => {
            append_bytes(out, &mut total, b"InitState:\t");
            append_u32_ascii(out, &mut total, init_state());
            append_bytes(out, &mut total, b"\nPmmTotal:\t");
            append_u32_ascii(out, &mut total, pmm_total_pages());
            append_bytes(out, &mut total, b"\nPmmFree:\t");
            append_u32_ascii(out, &mut total, pmm_free_pages());
            append_bytes(out, &mut total, b"\nSchedCount:\t");
            append_u32_ascii(out, &mut total, sched_count());
            append_bytes(out, &mut total, b"\nVfsCount:\t");
            append_u32_ascii(out, &mut total, vfs_count());
            append_bytes(out, &mut total, b"\nNetQueue:\t");
            append_u32_ascii(out, &mut total, net_queue_len());
            append_bytes(out, &mut total, b"\nDrivers:\t");
            append_u32_ascii(out, &mut total, drivers_count());
            append_bytes(out, &mut total, b"\nSyscalls:\t");
            append_u32_ascii(out, &mut total, syscall_count());
            append_bytes(out, &mut total, b"\nProcCount:\t");
            append_u32_ascii(out, &mut total, proc_count());
            append_bytes(out, &mut total, b"\n");
        }
        PROC_FD_NET_DEV => {
            let q = net_queue_len();
            append_bytes(out, &mut total, b"Inter-|   Receive                 |  Transmit\n");
            append_bytes(out, &mut total, b" eth0: ");
            append_u32_ascii(out, &mut total, q);
            append_bytes(out, &mut total, b" packets              ");
            append_u32_ascii(out, &mut total, q);
            append_bytes(out, &mut total, b" packets\n");
        }
        PROC_FD_SYSCALLS => {
            append_bytes(out, &mut total, b"SyscallCount:\t");
            append_u32_ascii(out, &mut total, syscall_count());
            append_bytes(out, &mut total, b"\n");
        }
        PROC_FD_UPTIME => {
            append_bytes(out, &mut total, b"0.00 0.00\n");
        }
        PROC_FD_RUNTIME_MAP => {
            let current_pid = sched_current();
            let current_tid = kthread_current_tid();
            let asid = vmm_get_asid();
            let map_count = vmm_mapping_count_as(asid);
            let proc_total = proc_count();
            let mut proc_name = [0u8; QOS_PROC_NAME_MAX];
            let mut thread_name = [0u8; QOS_KTHREAD_NAME_MAX];
            let proc_name_len = if current_pid == 0 {
                None
            } else {
                proc_name_get(current_pid, &mut proc_name)
            };
            let thread_name_len = if current_tid == 0 {
                None
            } else {
                kthread_name_get(current_tid, &mut thread_name)
            };

            append_bytes(out, &mut total, b"CurrentPid:\t");
            append_u32_ascii(out, &mut total, current_pid);
            append_bytes(out, &mut total, b"\nCurrentProc:\t");
            if let Some(len) = proc_name_len {
                append_bytes(out, &mut total, &proc_name[..len]);
            } else {
                append_bytes(out, &mut total, b"none");
            }
            append_bytes(out, &mut total, b"\nCurrentTid:\t");
            append_u32_ascii(out, &mut total, current_tid);
            append_bytes(out, &mut total, b"\nCurrentThread:\t");
            if let Some(len) = thread_name_len {
                append_bytes(out, &mut total, &thread_name[..len]);
            } else {
                append_bytes(out, &mut total, b"none");
            }
            append_bytes(out, &mut total, b"\nCurrentAsid:\t");
            append_u32_ascii(out, &mut total, asid);
            append_bytes(out, &mut total, b"\nMappings:\t");
            append_u32_ascii(out, &mut total, map_count);
            append_bytes(out, &mut total, b"\n");
            append_bytes(out, &mut total, b"Processes:\t");
            append_u32_ascii(out, &mut total, proc_total);
            append_bytes(out, &mut total, b"\n");

            let mut idx = 0u32;
            while idx < map_count {
                if let Some((va, pa, flags)) = vmm_mapping_get_as(asid, idx) {
                    append_bytes(out, &mut total, b"Map");
                    append_u32_ascii(out, &mut total, idx);
                    append_bytes(out, &mut total, b":\t0x");
                    append_u64_hex_fixed(out, &mut total, va, 16);
                    append_bytes(out, &mut total, b"->0x");
                    append_u64_hex_fixed(out, &mut total, pa, 16);
                    append_bytes(out, &mut total, b" f=0x");
                    append_u32_hex(out, &mut total, flags);
                    append_bytes(out, &mut total, b"\n");
                }
                idx += 1;
            }

            let mut pidx = 0u32;
            while pidx < proc_total {
                if let Some(pid) = proc_pid_at(pidx) {
                    let pmap_count = vmm_mapping_count_as(pid);
                    let mut pname = [0u8; QOS_PROC_NAME_MAX];
                    let pname_len = proc_name_get(pid, &mut pname);
                    append_bytes(out, &mut total, b"Proc");
                    append_u32_ascii(out, &mut total, pidx);
                    append_bytes(out, &mut total, b":\tpid=");
                    append_u32_ascii(out, &mut total, pid);
                    append_bytes(out, &mut total, b" name=");
                    if let Some(n) = pname_len {
                        append_bytes(out, &mut total, &pname[..n]);
                    } else {
                        append_bytes(out, &mut total, b"none");
                    }
                    append_bytes(out, &mut total, b" maps=");
                    append_u32_ascii(out, &mut total, pmap_count);
                    if pid == current_pid {
                        append_bytes(out, &mut total, b" current");
                    }
                    if pid == asid {
                        append_bytes(out, &mut total, b" asid");
                    }
                    append_bytes(out, &mut total, b"\n");

                    let mut midx = 0u32;
                    while midx < pmap_count {
                        if let Some((va, pa, flags)) = vmm_mapping_get_as(pid, midx) {
                            append_bytes(out, &mut total, b"  P");
                            append_u32_ascii(out, &mut total, pid);
                            append_bytes(out, &mut total, b"Map");
                            append_u32_ascii(out, &mut total, midx);
                            append_bytes(out, &mut total, b":\t0x");
                            append_u64_hex_fixed(out, &mut total, va, 16);
                            append_bytes(out, &mut total, b"->0x");
                            append_u64_hex_fixed(out, &mut total, pa, 16);
                            append_bytes(out, &mut total, b" f=0x");
                            append_u32_hex(out, &mut total, flags);
                            append_bytes(out, &mut total, b"\n");
                        }
                        midx += 1;
                    }
                }
                pidx += 1;
            }
        }
        PROC_FD_STATUS => {
            let pid = state.fd_proc_pid[fd_idx];
            let running = proc_alive(pid);
            append_bytes(out, &mut total, b"Pid:\t");
            append_u32_ascii(out, &mut total, pid);
            append_bytes(out, &mut total, b"\nState:\t");
            if running {
                append_bytes(out, &mut total, b"Running");
            } else {
                append_bytes(out, &mut total, b"Zombie");
            }
            append_bytes(out, &mut total, b"\n");
        }
        _ => {}
    }
    total
}

pub fn syscall_dispatch(nr: u32, a0: u64, a1: u64, a2: u64, a3: u64) -> i64 {
    if nr as usize >= QOS_SYSCALL_MAX {
        return -1;
    }

    syscall_lock();
    let ret = (|| {
        let state = syscall_state_mut();
        let idx = nr as usize;
        if state.used[idx] == 0 {
            return -1;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_CONST {
            return state.values[idx];
        }

        if state.ops[idx] as u32 == SYSCALL_OP_ADD {
            return (a0 + a1) as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_QUERY {
            return syscall_query_value(state.values[idx] as u32, state.count);
        }

        if state.ops[idx] as u32 == SYSCALL_OP_OPEN {
            if a0 == 0 {
                return -1;
            }
            let c_path = unsafe { CStr::from_ptr(a0 as *const core::ffi::c_char) };
            let path = match c_path.to_str() {
                Ok(p) => p,
                Err(_) => return -1,
            };
            if !vfs_exists(path) {
                let (proc_kind, proc_pid) = match syscall_proc_path_kind(path) {
                    Some(v) => v,
                    None => return -1,
                };
                if proc_kind == PROC_FD_STATUS && !proc_alive(proc_pid) {
                    return -1;
                }
                let fd = match syscall_fd_alloc(state, FD_KIND_PROC) {
                    Some(v) => v,
                    None => return -1,
                };
                state.fd_proc_kind[fd as usize] = proc_kind;
                state.fd_proc_pid[fd as usize] = proc_pid;
                return fd as i64;
            }
            let fd = match syscall_fd_alloc(state, FD_KIND_VFS) {
                Some(v) => v,
                None => return -1,
            };
            let file_id = match syscall_file_ensure(state, path) {
                Some(v) => v,
                None => {
                    syscall_fd_clear(state, fd);
                    return -1;
                }
            };
            state.fd_file_id[fd as usize] = file_id as u16;
            return fd as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_READ {
            let fd = a0 as u32;
            let len = a2 as usize;
            if !syscall_fd_valid(state, fd) || a1 == 0 {
                return -1;
            }
            let fd_idx = fd as usize;
            if state.fd_kind[fd_idx] == FD_KIND_PROC {
                let offset = state.fd_offset[fd_idx] as usize;
                let mut tmp = [0u8; 2048];
                let total = syscall_proc_render(state, fd_idx, &mut tmp);
                if offset >= total {
                    return 0;
                }
                let copy_len = core::cmp::min(len, total - offset);
                if copy_len == 0 {
                    return 0;
                }
                unsafe {
                    core::ptr::copy_nonoverlapping(
                        tmp.as_ptr().add(offset),
                        a1 as *mut u8,
                        copy_len,
                    );
                }
                state.fd_offset[fd_idx] = (offset + copy_len) as u64;
                return copy_len as i64;
            }
            if state.fd_kind[fd_idx] == FD_KIND_VFS {
                let file_id = state.fd_file_id[fd_idx] as usize;
                let offset = state.fd_offset[fd_idx] as usize;
                let file_len;
                if file_id >= QOS_VFS_MAX_NODES || state.file_used[file_id] == 0 {
                    return -1;
                }
                file_len = state.file_len[file_id] as usize;
                if offset >= file_len {
                    return 0;
                }
                let avail = file_len - offset;
                let copy_len = core::cmp::min(len, avail);
                if copy_len == 0 {
                    return 0;
                }
                unsafe {
                    core::ptr::copy_nonoverlapping(
                        state.file_data[file_id].as_ptr().add(offset),
                        a1 as *mut u8,
                        copy_len,
                    );
                }
                state.fd_offset[fd_idx] = (offset + copy_len) as u64;
                return copy_len as i64;
            }
            if state.fd_kind[fd_idx] == FD_KIND_PIPE_R {
                let pipe_id = state.fd_pipe_id[fd_idx] as usize;
                if pipe_id >= QOS_PIPE_MAX || state.pipe_used[pipe_id] == 0 || state.pipe_len[pipe_id] == 0 {
                    return -1;
                }
                let mut copy_len = state.pipe_len[pipe_id] as usize;
                if copy_len > len {
                    copy_len = len;
                }
                if copy_len == 0 {
                    return 0;
                }
                unsafe {
                    core::ptr::copy_nonoverlapping(
                        state.pipe_buf[pipe_id].as_ptr(),
                        a1 as *mut u8,
                        copy_len,
                    );
                }
                let remain = (state.pipe_len[pipe_id] as usize).saturating_sub(copy_len);
                let mut i = 0usize;
                while i < remain {
                    state.pipe_buf[pipe_id][i] = state.pipe_buf[pipe_id][i + copy_len];
                    i += 1;
                }
                while i < QOS_PIPE_CAP {
                    state.pipe_buf[pipe_id][i] = 0;
                    i += 1;
                }
                state.pipe_len[pipe_id] = remain as u16;
                return copy_len as i64;
            }
            if state.fd_kind[fd_idx] == FD_KIND_SOCKET {
                if state.fd_sock_connected[fd_idx] == 0 || len == 0 {
                    return -1;
                }
                if state.fd_sock_type[fd_idx] == SOCK_STREAM
                    && state.fd_sock_conn_id[fd_idx] < 0
                    && state.fd_sock_rip[fd_idx] == QOS_NET_IPV4_GATEWAY
                {
                    let got = state.fd_sock_stream_len[fd_idx] as usize;
                    if got == 0 {
                        return -1;
                    }
                    let copy_len = core::cmp::min(got, len);
                    let out = unsafe { core::slice::from_raw_parts_mut(a1 as *mut u8, len) };
                    out[..copy_len].copy_from_slice(&state.fd_sock_stream_buf[fd_idx][..copy_len]);
                    let remain = got - copy_len;
                    if remain != 0 {
                        let mut i = 0usize;
                        while i < remain {
                            state.fd_sock_stream_buf[fd_idx][i] = state.fd_sock_stream_buf[fd_idx][i + copy_len];
                            i += 1;
                        }
                    }
                    let mut i = remain;
                    while i < QOS_NET_MAX_PACKET {
                        state.fd_sock_stream_buf[fd_idx][i] = 0;
                        i += 1;
                    }
                    state.fd_sock_stream_len[fd_idx] = remain as u16;
                    return copy_len as i64;
                }
                if state.fd_sock_type[fd_idx] == SOCK_RAW {
                    let got = state.fd_sock_raw_len[fd_idx] as usize;
                    if got == 0 || got > len {
                        return -1;
                    }
                    let out = unsafe { core::slice::from_raw_parts_mut(a1 as *mut u8, len) };
                    out[..got].copy_from_slice(&state.fd_sock_raw_buf[fd_idx][..got]);
                    state.fd_sock_raw_len[fd_idx] = 0;
                    state.fd_sock_raw_src_ip[fd_idx] = 0;
                    state.fd_sock_raw_buf[fd_idx] = [0; QOS_NET_MAX_PACKET];
                    return got as i64;
                }
                if state.fd_sock_type[fd_idx] == SOCK_DGRAM {
                    let local_port = state.fd_sock_lport[fd_idx];
                    if local_port == 0 {
                        return -1;
                    }
                    let out = unsafe { core::slice::from_raw_parts_mut(a1 as *mut u8, len) };
                    return net_udp_recv(local_port, out).map_or(-1, |(n, _, _)| n as i64);
                }
                let out = unsafe { core::slice::from_raw_parts_mut(a1 as *mut u8, len) };
                return net_dequeue_packet(out).map_or(-1, |n| n as i64);
            }
            return -1;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_WRITE {
            let fd = a0 as u32;
            let len = a2 as usize;
            if !syscall_fd_valid(state, fd) || a1 == 0 {
                return -1;
            }
            let fd_idx = fd as usize;
            if state.fd_kind[fd_idx] == FD_KIND_PROC {
                return -1;
            }
            if state.fd_kind[fd_idx] == FD_KIND_VFS {
                let file_id = state.fd_file_id[fd_idx] as usize;
                let offset = state.fd_offset[fd_idx] as usize;
                if file_id >= QOS_VFS_MAX_NODES || state.file_used[file_id] == 0 {
                    return -1;
                }
                if offset >= QOS_PIPE_CAP {
                    return -1;
                }
                let write_len = core::cmp::min(len, QOS_PIPE_CAP - offset);
                if write_len == 0 {
                    return 0;
                }
                let src = unsafe { core::slice::from_raw_parts(a1 as *const u8, write_len) };
                state.file_data[file_id][offset..offset + write_len].copy_from_slice(src);
                let end = offset + write_len;
                state.fd_offset[fd_idx] = end as u64;
                if end > state.file_len[file_id] as usize {
                    state.file_len[file_id] = end as u16;
                }
                return write_len as i64;
            }
            if state.fd_kind[fd_idx] == FD_KIND_PIPE_W {
                let pipe_id = state.fd_pipe_id[fd_idx] as usize;
                if pipe_id >= QOS_PIPE_MAX || state.pipe_used[pipe_id] == 0 {
                    return -1;
                }
                let start = state.pipe_len[pipe_id] as usize;
                if len > QOS_PIPE_CAP.saturating_sub(start) {
                    return -1;
                }
                let src = unsafe { core::slice::from_raw_parts(a1 as *const u8, len) };
                state.pipe_buf[pipe_id][start..start + len].copy_from_slice(src);
                state.pipe_len[pipe_id] = (start + len) as u16;
                return len as i64;
            }
            if state.fd_kind[fd_idx] == FD_KIND_SOCKET {
                if state.fd_sock_connected[fd_idx] == 0 || len == 0 {
                    return -1;
                }
                let src = unsafe { core::slice::from_raw_parts(a1 as *const u8, len) };
                if state.fd_sock_type[fd_idx] == SOCK_STREAM
                    && state.fd_sock_conn_id[fd_idx] < 0
                    && state.fd_sock_rip[fd_idx] == QOS_NET_IPV4_GATEWAY
                {
                    if !syscall_stream_host_response_prepare(state, fd_idx, src) {
                        return -1;
                    }
                    return len as i64;
                }
                if state.fd_sock_type[fd_idx] == SOCK_RAW {
                    let ident = if state.fd_sock_lport[fd_idx] != 0 {
                        state.fd_sock_lport[fd_idx]
                    } else {
                        1
                    };
                    let remote_ip = state.fd_sock_rip[fd_idx];
                    if remote_ip == 0 || len > QOS_NET_MAX_PACKET {
                        return -1;
                    }
                    let mut reply = [0u8; QOS_NET_MAX_PACKET];
                    let (got, src_ip) = match net_icmp_echo(remote_ip, ident, 1, src, &mut reply) {
                        Some(v) => v,
                        None => return -1,
                    };
                    state.fd_sock_raw_buf[fd_idx] = reply;
                    state.fd_sock_raw_len[fd_idx] = got as u16;
                    state.fd_sock_raw_src_ip[fd_idx] = src_ip;
                    return len as i64;
                }
                if state.fd_sock_type[fd_idx] == SOCK_DGRAM {
                    let local_port = state.fd_sock_lport[fd_idx];
                    let remote_port = state.fd_sock_rport[fd_idx];
                    let remote_ip = state.fd_sock_rip[fd_idx];
                    if local_port == 0 || remote_port == 0 || remote_ip == 0 {
                        return -1;
                    }
                    return net_udp_send(local_port, QOS_NET_IPV4_LOCAL, remote_port, src).map_or(-1, |n| n as i64);
                }
                return if net_enqueue_packet(src) { len as i64 } else { -1 };
            }
            return -1;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_CLOSE {
            let fd = a0 as u32;
            if !syscall_fd_valid(state, fd) {
                return -1;
            }
            syscall_fd_clear(state, fd);
            return 0;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_DUP2 {
            let oldfd = a0 as u32;
            let newfd = a1 as u32;
            if !syscall_fd_valid(state, oldfd) || (newfd as usize) >= QOS_FD_MAX {
                return -1;
            }
            if oldfd == newfd {
                return newfd as i64;
            }
            if syscall_fd_valid(state, newfd) {
                syscall_fd_clear(state, newfd);
            }
            let old = oldfd as usize;
            let new = newfd as usize;
            state.fd_used[new] = 1;
            state.fd_kind[new] = state.fd_kind[old];
            state.fd_sock_bound[new] = state.fd_sock_bound[old];
            state.fd_sock_listening[new] = state.fd_sock_listening[old];
            state.fd_sock_connected[new] = state.fd_sock_connected[old];
            state.fd_sock_pending[new] = state.fd_sock_pending[old];
            state.fd_sock_backlog[new] = state.fd_sock_backlog[old];
            state.fd_sock_type[new] = state.fd_sock_type[old];
            state.fd_sock_lport[new] = state.fd_sock_lport[old];
            state.fd_sock_rport[new] = state.fd_sock_rport[old];
            state.fd_sock_rip[new] = state.fd_sock_rip[old];
            state.fd_sock_conn_id[new] = state.fd_sock_conn_id[old];
            state.fd_sock_stream_len[new] = state.fd_sock_stream_len[old];
            state.fd_sock_stream_buf[new] = state.fd_sock_stream_buf[old];
            state.fd_sock_raw_len[new] = state.fd_sock_raw_len[old];
            state.fd_sock_raw_src_ip[new] = state.fd_sock_raw_src_ip[old];
            state.fd_sock_raw_buf[new] = state.fd_sock_raw_buf[old];
            state.fd_sock_pending_rport[new] = state.fd_sock_pending_rport[old];
            state.fd_sock_pending_rip[new] = state.fd_sock_pending_rip[old];
            state.fd_sock_pending_conn_id[new] = state.fd_sock_pending_conn_id[old];
            state.fd_sock_pending_head[new] = state.fd_sock_pending_head[old];
            state.fd_sock_pending_tail[new] = state.fd_sock_pending_tail[old];
            state.fd_sock_pending_q_rport[new] = state.fd_sock_pending_q_rport[old];
            state.fd_sock_pending_q_rip[new] = state.fd_sock_pending_q_rip[old];
            state.fd_sock_pending_q_conn_id[new] = state.fd_sock_pending_q_conn_id[old];
            state.fd_pipe_id[new] = state.fd_pipe_id[old];
            state.fd_offset[new] = state.fd_offset[old];
            state.fd_file_id[new] = state.fd_file_id[old];
            state.fd_proc_kind[new] = state.fd_proc_kind[old];
            state.fd_proc_pid[new] = state.fd_proc_pid[old];
            return newfd as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_MMAP {
            let va = a0;
            let len = a1;
            let flags = if a2 == 0 { VM_READ } else { a2 as u32 };
            let mut off = 0u64;
            if va == 0 || len == 0 {
                return -1;
            }
            while off < len {
                if vmm_map(va + off, va + off, flags).is_err() {
                    while off > 0 {
                        off -= QOS_PAGE_SIZE;
                        let _ = vmm_unmap(va + off);
                    }
                    return -1;
                }
                off += QOS_PAGE_SIZE;
            }
            return va as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_MUNMAP {
            let va = a0;
            let len = a1;
            let mut off = 0u64;
            if va == 0 || len == 0 {
                return -1;
            }
            while off < len {
                if !vmm_unmap(va + off) {
                    return -1;
                }
                off += QOS_PAGE_SIZE;
            }
            return 0;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_YIELD {
            return sched_next().map_or(-1, |pid| pid as i64);
        }

        if state.ops[idx] as u32 == SYSCALL_OP_SLEEP {
            return 0;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_SOCKET {
            if a0 != 2 || (a1 as u8 != SOCK_STREAM && a1 as u8 != SOCK_DGRAM && a1 as u8 != SOCK_RAW) {
                return -1;
            }
            let fd = match syscall_fd_alloc(state, FD_KIND_SOCKET) {
                Some(v) => v,
                None => return -1,
            };
            state.fd_sock_type[fd as usize] = a1 as u8;
            return fd as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_BIND {
            let fd = a0 as u32;
            let (port, _ip) = match syscall_parse_sockaddr(a1, a2) {
                Some(v) => v,
                None => return -1,
            };
            if !syscall_fd_valid(state, fd)
                || state.fd_kind[fd as usize] != FD_KIND_SOCKET
                || state.fd_sock_bound[fd as usize] != 0
            {
                return -1;
            }
            let sock_type = state.fd_sock_type[fd as usize];
            if sock_type == SOCK_RAW {
                state.fd_sock_bound[fd as usize] = 1;
                state.fd_sock_lport[fd as usize] = port;
                return 0;
            }
            if !syscall_sock_port_mark(state, sock_type, port) {
                return -1;
            }
            if sock_type == SOCK_DGRAM && !net_udp_bind(port) {
                syscall_sock_port_unmark(state, sock_type, port);
                return -1;
            }
            state.fd_sock_bound[fd as usize] = 1;
            state.fd_sock_lport[fd as usize] = port;
            return 0;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_LISTEN {
            let fd = a0 as u32;
            let backlog = core::cmp::min(a1 as usize, QOS_SOCK_PENDING_MAX) as u8;
            if !syscall_fd_valid(state, fd)
                || state.fd_kind[fd as usize] != FD_KIND_SOCKET
                || state.fd_sock_type[fd as usize] != SOCK_STREAM
                || state.fd_sock_bound[fd as usize] == 0
                || state.fd_sock_lport[fd as usize] == 0
                || a1 == 0
            {
                return -1;
            }
            if backlog == 0 {
                return -1;
            }
            if net_tcp_listen(state.fd_sock_lport[fd as usize]).is_none() {
                return -1;
            }
            state.fd_sock_listening[fd as usize] = 1;
            state.fd_sock_backlog[fd as usize] = backlog;
            return 0;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_ACCEPT {
            let fd = a0 as u32;
            if !syscall_fd_valid(state, fd)
                || state.fd_kind[fd as usize] != FD_KIND_SOCKET
                || state.fd_sock_type[fd as usize] != SOCK_STREAM
                || state.fd_sock_listening[fd as usize] == 0
                || state.fd_sock_pending[fd as usize] == 0
            {
                return -1;
            }
            let newfd = match syscall_fd_alloc(state, FD_KIND_SOCKET) {
                Some(v) => v as usize,
                None => return -1,
            };
            let (peer_port, peer_ip, peer_conn_id) = match syscall_sock_pending_pop(state, fd) {
                Some(v) => v,
                None => {
                    syscall_fd_clear(state, newfd as u32);
                    return -1;
                }
            };
            state.fd_sock_bound[newfd] = 1;
            state.fd_sock_connected[newfd] = 1;
            state.fd_sock_type[newfd] = SOCK_STREAM;
            state.fd_sock_lport[newfd] = state.fd_sock_lport[fd as usize];
            state.fd_sock_rport[newfd] = peer_port;
            state.fd_sock_rip[newfd] = peer_ip;
            state.fd_sock_conn_id[newfd] = peer_conn_id;
            state.fd_sock_pending_rport[fd as usize] = peer_port;
            state.fd_sock_pending_rip[fd as usize] = peer_ip;
            state.fd_sock_pending_conn_id[fd as usize] = peer_conn_id;
            if a1 != 0 && a2 != 0 {
                unsafe {
                    let addrlen_ptr = a2 as *mut u32;
                    if *addrlen_ptr >= 16 {
                        let out = core::slice::from_raw_parts_mut(a1 as *mut u8, 16);
                        out.fill(0);
                        out[0] = 2;
                        out[1] = 0;
                        out[2] = (peer_port >> 8) as u8;
                        out[3] = (peer_port & 0xFF) as u8;
                        out[4] = (peer_ip >> 24) as u8;
                        out[5] = ((peer_ip >> 16) & 0xFF) as u8;
                        out[6] = ((peer_ip >> 8) & 0xFF) as u8;
                        out[7] = (peer_ip & 0xFF) as u8;
                        *addrlen_ptr = 16;
                    } else {
                        *addrlen_ptr = 0;
                    }
                }
            } else if a2 != 0 {
                unsafe {
                    *(a2 as *mut u32) = 0;
                }
            }
            return newfd as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_CONNECT {
            let fd = a0 as u32;
            let (remote_port, remote_ip) = match syscall_parse_sockaddr(a1, a2) {
                Some(v) => v,
                None => return -1,
            };
            if !syscall_fd_valid(state, fd) || state.fd_kind[fd as usize] != FD_KIND_SOCKET {
                return -1;
            }
            if state.fd_sock_type[fd as usize] == SOCK_STREAM {
                let mut listener: Option<usize> = None;
                let mut i = 0usize;
                let mut local_port = state.fd_sock_lport[fd as usize];
                let mut allocated_local = false;
                while i < QOS_FD_MAX {
                    if i != fd as usize
                        && state.fd_used[i] != 0
                        && state.fd_kind[i] == FD_KIND_SOCKET
                        && state.fd_sock_type[i] == SOCK_STREAM
                        && state.fd_sock_listening[i] != 0
                        && state.fd_sock_lport[i] == remote_port
                    {
                        listener = Some(i);
                        break;
                    }
                    i += 1;
                }
                let listener_idx = match listener {
                    Some(v) => v,
                    None => {
                        if remote_ip != QOS_NET_IPV4_GATEWAY {
                            return -1;
                        }
                        if local_port == 0 {
                            local_port = match syscall_sock_port_alloc(state, SOCK_STREAM) {
                                Some(v) => v,
                                None => return -1,
                            };
                        }
                        state.fd_sock_bound[fd as usize] = 1;
                        state.fd_sock_lport[fd as usize] = local_port;
                        state.fd_sock_connected[fd as usize] = 1;
                        state.fd_sock_rport[fd as usize] = remote_port;
                        state.fd_sock_rip[fd as usize] = remote_ip;
                        state.fd_sock_conn_id[fd as usize] = -1;
                        return 0;
                    }
                };
                let listener_backlog = state.fd_sock_backlog[listener_idx] as usize;
                if listener_backlog == 0 || state.fd_sock_pending[listener_idx] as usize >= listener_backlog {
                    return -1;
                }
                if local_port == 0 {
                    local_port = match syscall_sock_port_alloc(state, SOCK_STREAM) {
                        Some(v) => v,
                        None => return -1,
                    };
                    allocated_local = true;
                }
                let conn_id = match net_tcp_connect(local_port, remote_ip, remote_port) {
                    Some(v) => v as i16,
                    None => {
                        if allocated_local {
                            syscall_sock_port_unmark(state, SOCK_STREAM, local_port);
                        }
                        return -1;
                    }
                };
                if !net_tcp_step(conn_id as u16, QOS_TCP_EVT_RX_SYN_ACK) {
                    if allocated_local {
                        syscall_sock_port_unmark(state, SOCK_STREAM, local_port);
                    }
                    return -1;
                }
                state.fd_sock_bound[fd as usize] = 1;
                state.fd_sock_lport[fd as usize] = local_port;
                state.fd_sock_connected[fd as usize] = 1;
                state.fd_sock_rport[fd as usize] = remote_port;
                state.fd_sock_rip[fd as usize] = remote_ip;
                state.fd_sock_conn_id[fd as usize] = conn_id;
                if !syscall_sock_pending_push(state, listener_idx as u32, local_port, QOS_NET_IPV4_LOCAL, conn_id) {
                    if allocated_local {
                        syscall_sock_port_unmark(state, SOCK_STREAM, local_port);
                    }
                    return -1;
                }
                return 0;
            }
            if state.fd_sock_type[fd as usize] == SOCK_DGRAM {
                let mut local_port = state.fd_sock_lport[fd as usize];
                if local_port == 0 {
                    local_port = match syscall_sock_port_alloc(state, SOCK_DGRAM) {
                        Some(v) => v,
                        None => return -1,
                    };
                    if !net_udp_bind(local_port) {
                        syscall_sock_port_unmark(state, SOCK_DGRAM, local_port);
                        return -1;
                    }
                    state.fd_sock_bound[fd as usize] = 1;
                    state.fd_sock_lport[fd as usize] = local_port;
                }
                state.fd_sock_connected[fd as usize] = 1;
                state.fd_sock_rport[fd as usize] = remote_port;
                state.fd_sock_rip[fd as usize] = remote_ip;
                return 0;
            }
            if state.fd_sock_type[fd as usize] == SOCK_RAW {
                let mut local_port = state.fd_sock_lport[fd as usize];
                if local_port == 0 {
                    local_port = 1;
                }
                state.fd_sock_bound[fd as usize] = 1;
                state.fd_sock_lport[fd as usize] = local_port;
                state.fd_sock_connected[fd as usize] = 1;
                state.fd_sock_rport[fd as usize] = remote_port;
                state.fd_sock_rip[fd as usize] = remote_ip;
                return 0;
            }
            return -1;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_SEND {
            let fd = a0 as u32;
            let len = a2 as usize;
            if !syscall_fd_valid(state, fd)
                || state.fd_kind[fd as usize] != FD_KIND_SOCKET
                || a1 == 0
                || len == 0
            {
                return -1;
            }
            if state.fd_sock_type[fd as usize] == SOCK_STREAM && state.fd_sock_connected[fd as usize] == 0 {
                return -1;
            }
            let src = unsafe { core::slice::from_raw_parts(a1 as *const u8, len) };
            if state.fd_sock_type[fd as usize] == SOCK_RAW {
                let ident = if state.fd_sock_lport[fd as usize] != 0 {
                    state.fd_sock_lport[fd as usize]
                } else {
                    1
                };
                let mut remote_port = state.fd_sock_rport[fd as usize];
                let mut remote_ip = state.fd_sock_rip[fd as usize];
                if state.fd_sock_connected[fd as usize] == 0 {
                    let (port, ip) = match syscall_parse_sockaddr(a3, 16) {
                        Some(v) => v,
                        None => return -1,
                    };
                    remote_port = port;
                    remote_ip = ip;
                }
                if remote_ip == 0 || len > QOS_NET_MAX_PACKET {
                    return -1;
                }
                let mut reply = [0u8; QOS_NET_MAX_PACKET];
                let (got, src_ip) = match net_icmp_echo(remote_ip, ident, 1, src, &mut reply) {
                    Some(v) => v,
                    None => return -1,
                };
                state.fd_sock_rport[fd as usize] = remote_port;
                state.fd_sock_rip[fd as usize] = remote_ip;
                state.fd_sock_raw_buf[fd as usize] = reply;
                state.fd_sock_raw_len[fd as usize] = got as u16;
                state.fd_sock_raw_src_ip[fd as usize] = src_ip;
                return len as i64;
            }
            if state.fd_sock_type[fd as usize] == SOCK_DGRAM {
                let mut local_port = state.fd_sock_lport[fd as usize];
                let mut remote_port = state.fd_sock_rport[fd as usize];
                let mut remote_ip = state.fd_sock_rip[fd as usize];
                if state.fd_sock_connected[fd as usize] == 0 {
                    let (port, ip) = match syscall_parse_sockaddr(a3, 16) {
                        Some(v) => v,
                        None => return -1,
                    };
                    remote_port = port;
                    remote_ip = ip;
                }
                if local_port == 0 {
                    local_port = match syscall_sock_port_alloc(state, SOCK_DGRAM) {
                        Some(v) => v,
                        None => return -1,
                    };
                    if !net_udp_bind(local_port) {
                        syscall_sock_port_unmark(state, SOCK_DGRAM, local_port);
                        return -1;
                    }
                    state.fd_sock_bound[fd as usize] = 1;
                    state.fd_sock_lport[fd as usize] = local_port;
                }
                if remote_port == 0 || remote_ip == 0 {
                    return -1;
                }
                state.fd_sock_rport[fd as usize] = remote_port;
                state.fd_sock_rip[fd as usize] = remote_ip;
                return net_udp_send(local_port, QOS_NET_IPV4_LOCAL, remote_port, src).map_or(-1, |n| n as i64);
            }
            if state.fd_sock_type[fd as usize] == SOCK_STREAM
                && state.fd_sock_conn_id[fd as usize] < 0
                && state.fd_sock_rip[fd as usize] == QOS_NET_IPV4_GATEWAY
            {
                if !syscall_stream_host_response_prepare(state, fd as usize, src) {
                    return -1;
                }
                return len as i64;
            }
            return if net_enqueue_packet(src) { len as i64 } else { -1 };
        }

        if state.ops[idx] as u32 == SYSCALL_OP_RECV {
            let fd = a0 as u32;
            let len = a2 as usize;
            if !syscall_fd_valid(state, fd)
                || state.fd_kind[fd as usize] != FD_KIND_SOCKET
                || a1 == 0
                || len == 0
            {
                return -1;
            }
            if state.fd_sock_type[fd as usize] == SOCK_STREAM && state.fd_sock_connected[fd as usize] == 0 {
                return -1;
            }
            let out = unsafe { core::slice::from_raw_parts_mut(a1 as *mut u8, len) };
            if state.fd_sock_type[fd as usize] == SOCK_RAW {
                let got = state.fd_sock_raw_len[fd as usize] as usize;
                if got == 0 || got > len {
                    return -1;
                }
                out[..got].copy_from_slice(&state.fd_sock_raw_buf[fd as usize][..got]);
                syscall_write_sockaddr(a3, 1, state.fd_sock_raw_src_ip[fd as usize]);
                state.fd_sock_raw_len[fd as usize] = 0;
                state.fd_sock_raw_src_ip[fd as usize] = 0;
                state.fd_sock_raw_buf[fd as usize] = [0; QOS_NET_MAX_PACKET];
                return got as i64;
            }
            if state.fd_sock_type[fd as usize] == SOCK_DGRAM {
                let local_port = state.fd_sock_lport[fd as usize];
                if local_port == 0 {
                    return -1;
                }
                return net_udp_recv(local_port, out).map_or(-1, |(n, src_ip, src_port)| {
                    syscall_write_sockaddr(a3, src_port, src_ip);
                    n as i64
                });
            }
            if state.fd_sock_type[fd as usize] == SOCK_STREAM
                && state.fd_sock_conn_id[fd as usize] < 0
                && state.fd_sock_rip[fd as usize] == QOS_NET_IPV4_GATEWAY
            {
                let got = state.fd_sock_stream_len[fd as usize] as usize;
                if got == 0 {
                    return -1;
                }
                let copy_len = core::cmp::min(got, len);
                out[..copy_len].copy_from_slice(&state.fd_sock_stream_buf[fd as usize][..copy_len]);
                let remain = got - copy_len;
                if remain != 0 {
                    let mut i = 0usize;
                    while i < remain {
                        state.fd_sock_stream_buf[fd as usize][i] = state.fd_sock_stream_buf[fd as usize][i + copy_len];
                        i += 1;
                    }
                }
                let mut i = remain;
                while i < QOS_NET_MAX_PACKET {
                    state.fd_sock_stream_buf[fd as usize][i] = 0;
                    i += 1;
                }
                state.fd_sock_stream_len[fd as usize] = remain as u16;
                return copy_len as i64;
            }
            return net_dequeue_packet(out).map_or(-1, |n| n as i64);
        }

        if state.ops[idx] as u32 == SYSCALL_OP_GETDENTS {
            let fd = a0 as u32;
            if !syscall_fd_valid(state, fd) || state.fd_kind[fd as usize] != FD_KIND_VFS || a1 == 0 {
                return -1;
            }
            if a2 < core::mem::size_of::<u32>() as u64 {
                return -1;
            }
            let count = vfs_count();
            unsafe {
                *(a1 as *mut u32) = count;
            }
            return count as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_LSEEK {
            let fd = a0 as u32;
            let offset = a1 as i64;
            let whence = a2 as u32;
            if !syscall_fd_valid(state, fd)
                || (state.fd_kind[fd as usize] != FD_KIND_VFS && state.fd_kind[fd as usize] != FD_KIND_PROC)
            {
                return -1;
            }
            let base = if whence == 0 {
                0i64
            } else if whence == 1 {
                state.fd_offset[fd as usize] as i64
            } else if whence == 2 {
                if state.fd_kind[fd as usize] == FD_KIND_PROC {
                    let mut tmp = [0u8; 2048];
                    syscall_proc_render(state, fd as usize, &mut tmp) as i64
                } else {
                    let file_id = state.fd_file_id[fd as usize] as usize;
                    if file_id >= QOS_VFS_MAX_NODES || state.file_used[file_id] == 0 {
                        return -1;
                    }
                    state.file_len[file_id] as i64
                }
            } else {
                return -1;
            };
            let next = match base.checked_add(offset) {
                Some(v) => v,
                None => return -1,
            };
            if next < 0 {
                return -1;
            }
            state.fd_offset[fd as usize] = next as u64;
            return next;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_PIPE {
            if a0 == 0 {
                return -1;
            }
            let pipe_id = match syscall_pipe_alloc(state) {
                Some(v) => v,
                None => return -1,
            };
            let rfd = match syscall_fd_alloc(state, FD_KIND_PIPE_R) {
                Some(v) => v,
                None => {
                    state.pipe_used[pipe_id as usize] = 0;
                    return -1;
                }
            };
            let wfd = match syscall_fd_alloc(state, FD_KIND_PIPE_W) {
                Some(v) => v,
                None => {
                    syscall_fd_clear(state, rfd);
                    state.pipe_used[pipe_id as usize] = 0;
                    return -1;
                }
            };
            state.fd_pipe_id[rfd as usize] = pipe_id;
            state.fd_pipe_id[wfd as usize] = pipe_id;
            unsafe {
                let out = a0 as *mut u32;
                *out = rfd;
                *out.add(1) = wfd;
            }
            return 0;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_KILL {
            return if proc_signal_send(a0 as u32, a1 as u32) { 0 } else { -1 };
        }

        if state.ops[idx] as u32 == SYSCALL_OP_SIGPENDING {
            return proc_signal_pending(a0 as u32).map_or(-1, |p| p as i64);
        }

        if state.ops[idx] as u32 == SYSCALL_OP_SIGACTION {
            let old = match proc_signal_get_handler(a0 as u32, a1 as u32) {
                Some(v) => v,
                None => return -1,
            };
            if a2 != u64::MAX && !proc_signal_set_handler(a0 as u32, a1 as u32, a2 as u32) {
                return -1;
            }
            return old as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_SIGPROCMASK {
            let old = match proc_signal_mask(a0 as u32) {
                Some(v) => v,
                None => return -1,
            };
            let how = a1 as u32;
            let mask = a2 as u32;
            let new_mask = if how == SIG_BLOCK {
                old | mask
            } else if how == SIG_UNBLOCK {
                old & !mask
            } else if how == SIG_SETMASK {
                mask
            } else {
                return -1;
            };
            if !proc_signal_set_mask(a0 as u32, new_mask) {
                return -1;
            }
            return old as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_SIGALTSTACK {
            let old = match proc_signal_get_altstack(a0 as u32) {
                Some(v) => v,
                None => return -1,
            };
            if a1 != u64::MAX && !proc_signal_set_altstack(a0 as u32, a1, a2, a3 as u32) {
                return -1;
            }
            return old.flags as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_SIGSUSPEND {
            let old = match proc_signal_mask(a0 as u32) {
                Some(v) => v,
                None => return -1,
            };
            if !proc_signal_set_mask(a0 as u32, a1 as u32) {
                return -1;
            }
            let delivered = proc_signal_next(a0 as u32);
            if !proc_signal_set_mask(a0 as u32, old) {
                return -1;
            }
            return if delivered < 0 { -1 } else { delivered as i64 };
        }

        if state.ops[idx] as u32 == SYSCALL_OP_FORK {
            return if proc_fork(a0 as u32, a1 as u32) {
                a1 as i64
            } else {
                -1
            };
        }

        if state.ops[idx] as u32 == SYSCALL_OP_EXEC {
            if a1 == 0 {
                return if proc_exec_signal_reset(a0 as u32) { 0 } else { -1 };
            }
            let c_path = unsafe { CStr::from_ptr(a1 as *const core::ffi::c_char) };
            let path = match c_path.to_str() {
                Ok(p) => p,
                Err(_) => return -1,
            };
            let file_id = match syscall_file_find(state, path) {
                Some(i) => i,
                None => return -1,
            };
            let len = state.file_len[file_id] as usize;
            if len == 0 {
                return -1;
            }
            let image = match parse_exec_elf_image(&state.file_data[file_id][..len]) {
                Some(v) => v,
                None => return -1,
            };
            if !proc_exec_signal_reset(a0 as u32) || !proc_exec_image_set(a0 as u32, image) {
                return -1;
            }
            return 0;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_DLOPEN {
            if a0 == 0 {
                return -1;
            }
            let c_path = unsafe { CStr::from_ptr(a0 as *const core::ffi::c_char) };
            let path = match c_path.to_str() {
                Ok(p) => p,
                Err(_) => return -1,
            };
            return syscall_shlib_load(state, path, false).map_or(-1, |h| h as i64);
        }

        if state.ops[idx] as u32 == SYSCALL_OP_DLCLOSE {
            return if syscall_shlib_release(state, a0 as u32) { 0 } else { -1 };
        }

        if state.ops[idx] as u32 == SYSCALL_OP_DLSYM {
            if a1 == 0 {
                return -1;
            }
            let handle = a0 as u32;
            if syscall_shlib_find_handle(state, handle).is_none() {
                return -1;
            }
            let c_name = unsafe { CStr::from_ptr(a1 as *const core::ffi::c_char) };
            let name = match c_name.to_str() {
                Ok(v) => v,
                Err(_) => return -1,
            };
            let hash = symbol_hash32(name);
            if hash == 0 {
                return -1;
            }
            let addr = 0x7000_0000_0000_0000u64 | ((handle as u64) << 24) | ((hash & 0x00FF_FFFF) as u64);
            return addr as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_MODLOAD {
            if a0 == 0 {
                return -1;
            }
            let c_path = unsafe { CStr::from_ptr(a0 as *const core::ffi::c_char) };
            let path = match c_path.to_str() {
                Ok(p) => p,
                Err(_) => return -1,
            };
            return syscall_module_load(state, path).map_or(-1, |id| id as i64);
        }

        if state.ops[idx] as u32 == SYSCALL_OP_MODUNLOAD {
            return if syscall_module_unload(state, a0 as u32) { 0 } else { -1 };
        }

        if state.ops[idx] as u32 == SYSCALL_OP_MODRELOAD {
            return syscall_module_reload(state, a0 as u32).map_or(-1, |generation| generation as i64);
        }

        if state.ops[idx] as u32 == SYSCALL_OP_EXIT {
            return if proc_exit(a0 as u32, a1 as i32) { 0 } else { -1 };
        }

        if state.ops[idx] as u32 == SYSCALL_OP_GETPID {
            return if proc_alive(a0 as u32) { a0 as i64 } else { -1 };
        }

        if state.ops[idx] as u32 == SYSCALL_OP_WAITPID {
            let (pid, status, has_status) = proc_wait_status(a0 as u32, a1 as i32, a3 as u32);
            if has_status && a2 != 0 {
                unsafe {
                    *(a2 as *mut i32) = status;
                }
            }
            return pid as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_STAT
            || state.ops[idx] as u32 == SYSCALL_OP_MKDIR
            || state.ops[idx] as u32 == SYSCALL_OP_UNLINK
        {
            if a0 == 0 {
                return -1;
            }
            let c_path = unsafe { CStr::from_ptr(a0 as *const core::ffi::c_char) };
            let path = match c_path.to_str() {
                Ok(p) => p,
                Err(_) => return -1,
            };
            if state.ops[idx] as u32 == SYSCALL_OP_STAT {
                let mut kind = 1u64;
                if !vfs_exists(path) {
                    let (proc_kind, proc_pid) = match syscall_proc_path_kind(path) {
                        Some(v) => v,
                        None => return -1,
                    };
                    if proc_kind == PROC_FD_STATUS && !proc_alive(proc_pid) {
                        return -1;
                    }
                    kind = 2;
                }
                if a1 != 0 {
                    unsafe {
                        *(a1 as *mut u64) = path.len() as u64;
                        *((a1 as *mut u64).add(1)) = kind;
                    }
                }
                return 0;
            }
            if state.ops[idx] as u32 == SYSCALL_OP_MKDIR {
                if vfs_fs_kind(path) == Some(QOS_VFS_FS_PROCFS) {
                    return -1;
                }
                return if vfs_create(path) { 0 } else { -1 };
            }
            if vfs_fs_kind(path) == Some(QOS_VFS_FS_PROCFS) {
                return -1;
            }
            if !vfs_remove(path) {
                return -1;
            }
            if let Some(file_id) = syscall_file_find(state, path) {
                state.file_paths[file_id] = [0; QOS_VFS_PATH_MAX];
                state.file_path_len[file_id] = 0;
                syscall_file_maybe_gc(state, file_id);
            }
            return 0;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_CHDIR {
            if a1 == 0 {
                return -1;
            }
            let c_path = unsafe { CStr::from_ptr(a1 as *const core::ffi::c_char) };
            let path = match c_path.to_str() {
                Ok(p) => p,
                Err(_) => return -1,
            };
            if !vfs_exists(path) {
                return -1;
            }
            return if proc_cwd_set(a0 as u32, path) { 0 } else { -1 };
        }

        if state.ops[idx] as u32 == SYSCALL_OP_GETCWD {
            let pid = a0 as u32;
            if a1 == 0 {
                return proc_cwd_len(pid).map_or(-1, |n| n as i64);
            }
            if a2 == 0 {
                return -1;
            }
            let (cwd, len) = match proc_cwd_snapshot(pid) {
                Some(v) => v,
                None => return -1,
            };
            if len + 1 > a2 as usize {
                return -1;
            }
            unsafe {
                let out = a1 as *mut u8;
                let mut i = 0usize;
                while i <= len {
                    *out.add(i) = cwd[i];
                    i += 1;
                }
            }
            return len as i64;
        }

        if state.ops[idx] as u32 == SYSCALL_OP_SIGRETURN {
            return if proc_signal_set_mask(a0 as u32, a1 as u32) {
                0
            } else {
                -1
            };
        }

        -1
    })();
    syscall_unlock();
    ret
}

pub fn syscall_count() -> u32 {
    syscall_lock();
    let count = syscall_state().count;
    syscall_unlock();
    count
}

fn proc_find(state: &ProcState, pid: u32) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_PROC_MAX {
        if state.used[i] != 0 && state.pids[i] == pid {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn proc_free_slot(state: &ProcState) -> Option<usize> {
    let mut i = 0usize;
    while i < QOS_PROC_MAX {
        if state.used[i] == 0 {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn proc_signal_signum_valid(signum: u32) -> bool {
    signum > 0 && (signum as usize) < QOS_SIGNAL_MAX
}

fn proc_signal_bit(signum: u32) -> u32 {
    1u32 << signum
}

fn proc_signal_unmaskable_bits() -> u32 {
    proc_signal_bit(QOS_SIGKILL) | proc_signal_bit(QOS_SIGSTOP)
}

fn proc_cwd_set_idx(state: &mut ProcState, idx: usize, path: &str) -> bool {
    let bytes = path.as_bytes();
    if bytes.is_empty() || bytes[0] != b'/' || bytes.len() >= QOS_VFS_PATH_MAX {
        return false;
    }
    let mut i = 0usize;
    while i < bytes.len() {
        state.cwd[idx][i] = bytes[i];
        i += 1;
    }
    state.cwd[idx][bytes.len()] = 0;
    while i + 1 < QOS_VFS_PATH_MAX {
        state.cwd[idx][i + 1] = 0;
        i += 1;
    }
    state.cwd_len[idx] = bytes.len() as u8;
    true
}

fn write_u32_ascii(out: &mut [u8], value: u32) -> usize {
    if out.is_empty() {
        return 0;
    }
    if value == 0 {
        out[0] = b'0';
        return 1;
    }
    let mut tmp = [0u8; 16];
    let mut n = 0usize;
    let mut v = value;
    while v > 0 && n < tmp.len() {
        tmp[n] = b'0' + (v % 10) as u8;
        v /= 10;
        n += 1;
    }
    let mut i = 0usize;
    while i < n && i < out.len() {
        out[i] = tmp[n - 1 - i];
        i += 1;
    }
    i
}

fn proc_name_init_idx(state: &mut ProcState, idx: usize, pid: u32) {
    let prefix = b"proc-";
    let mut off = 0usize;
    while off < prefix.len() && off < (QOS_PROC_NAME_MAX.saturating_sub(1)) {
        state.names[idx][off] = prefix[off];
        off += 1;
    }
    if off < (QOS_PROC_NAME_MAX.saturating_sub(1)) {
        off += write_u32_ascii(&mut state.names[idx][off..QOS_PROC_NAME_MAX - 1], pid);
    }
    if off >= QOS_PROC_NAME_MAX {
        off = QOS_PROC_NAME_MAX - 1;
    }
    state.names[idx][off] = 0;
    state.name_len[idx] = off as u8;
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ProcExecImage {
    pub entry: u64,
    pub phoff: u64,
    pub phentsize: u16,
    pub phnum: u16,
    pub load_count: u16,
    pub has_interp: bool,
    pub interp_off: u64,
    pub interp_len: u64,
}

fn proc_exec_image_clear_idx(state: &mut ProcState, idx: usize) {
    state.exec_entry[idx] = 0;
    state.exec_phoff[idx] = 0;
    state.exec_phentsize[idx] = 0;
    state.exec_phnum[idx] = 0;
    state.exec_load_count[idx] = 0;
    state.exec_has_interp[idx] = 0;
    state.exec_interp_off[idx] = 0;
    state.exec_interp_len[idx] = 0;
}

pub fn proc_reset() {
    proc_lock();
    zero_struct(proc_state_mut());
    proc_unlock();
}

pub fn proc_create(pid: u32, ppid: u32) -> bool {
    if pid == 0 {
        return false;
    }

    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        if proc_find(state, pid).is_some() {
            return false;
        }
        let slot = match proc_free_slot(state) {
            Some(s) => s,
            None => return false,
        };
        state.used[slot] = 1;
        state.exited[slot] = 0;
        state.exit_code[slot] = 0;
        state.pids[slot] = pid;
        state.ppids[slot] = ppid;
        if !proc_cwd_set_idx(state, slot, "/") {
            return false;
        }
        proc_name_init_idx(state, slot, pid);
        state.sig_handlers[slot] = [QOS_SIG_DFL; QOS_SIGNAL_MAX];
        state.sig_pending[slot] = 0;
        state.sig_blocked[slot] = 0;
        state.sig_altstack_sp[slot] = 0;
        state.sig_altstack_size[slot] = 0;
        state.sig_altstack_flags[slot] = 0;
        proc_exec_image_clear_idx(state, slot);
        state.count += 1;
        true
    })();
    proc_unlock();
    ok
}

pub fn proc_remove(pid: u32) -> bool {
    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return false,
        };
        state.used[idx] = 0;
        state.exited[idx] = 0;
        state.exit_code[idx] = 0;
        state.pids[idx] = 0;
        state.ppids[idx] = 0;
        state.cwd[idx] = [0; QOS_VFS_PATH_MAX];
        state.cwd_len[idx] = 0;
        state.names[idx] = [0; QOS_PROC_NAME_MAX];
        state.name_len[idx] = 0;
        state.sig_handlers[idx] = [QOS_SIG_DFL; QOS_SIGNAL_MAX];
        state.sig_pending[idx] = 0;
        state.sig_blocked[idx] = 0;
        state.sig_altstack_sp[idx] = 0;
        state.sig_altstack_size[idx] = 0;
        state.sig_altstack_flags[idx] = 0;
        proc_exec_image_clear_idx(state, idx);
        state.count -= 1;
        true
    })();
    proc_unlock();
    ok
}

pub fn proc_parent(pid: u32) -> Option<u32> {
    proc_lock();
    let parent = (|| {
        let state = proc_state();
        proc_find(state, pid).map(|idx| state.ppids[idx])
    })();
    proc_unlock();
    parent
}

pub fn proc_alive(pid: u32) -> bool {
    proc_lock();
    let alive = (|| {
        let state = proc_state();
        match proc_find(state, pid) {
            Some(idx) => state.exited[idx] == 0,
            None => false,
        }
    })();
    proc_unlock();
    alive
}

pub fn proc_count() -> u32 {
    proc_lock();
    let count = proc_state().count;
    proc_unlock();
    count
}

pub fn proc_pid_at(ordinal: u32) -> Option<u32> {
    proc_lock();
    let pid = {
        let state = proc_state();
        let mut i = 0usize;
        let mut seen = 0u32;
        let mut out = None;
        while i < QOS_PROC_MAX {
            if state.used[i] != 0 {
                if seen == ordinal {
                    out = Some(state.pids[i]);
                    break;
                }
                seen = seen.saturating_add(1);
            }
            i += 1;
        }
        out
    };
    proc_unlock();
    pid
}

pub fn proc_name_get(pid: u32, out: &mut [u8]) -> Option<usize> {
    if out.is_empty() {
        return None;
    }
    proc_lock();
    let copied = (|| {
        let state = proc_state();
        let idx = proc_find(state, pid)?;
        let len = state.name_len[idx] as usize;
        if len + 1 > out.len() {
            return None;
        }
        let mut i = 0usize;
        while i <= len {
            out[i] = state.names[idx][i];
            i += 1;
        }
        Some(len)
    })();
    proc_unlock();
    copied
}

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct SpinLock {
    locked: bool,
}

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct KernelMutex {
    locked: bool,
    waiters: u32,
}

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct KernelSemaphore {
    value: i32,
    waiters: u32,
}

pub fn spin_init(lock: &mut SpinLock) {
    lock.locked = false;
}

pub fn spin_trylock(lock: &mut SpinLock) -> bool {
    if lock.locked {
        false
    } else {
        lock.locked = true;
        true
    }
}

pub fn spin_lock(lock: &mut SpinLock) -> bool {
    spin_trylock(lock)
}

pub fn spin_unlock(lock: &mut SpinLock) -> bool {
    if !lock.locked {
        return false;
    }
    lock.locked = false;
    true
}

pub fn spin_is_locked(lock: &SpinLock) -> bool {
    lock.locked
}

pub fn mutex_init(mutex: &mut KernelMutex) {
    mutex.locked = false;
    mutex.waiters = 0;
}

pub fn mutex_lock(mutex: &mut KernelMutex) -> bool {
    if !mutex.locked {
        mutex.locked = true;
        return true;
    }
    mutex.waiters += 1;
    false
}

pub fn mutex_trylock(mutex: &mut KernelMutex) -> bool {
    if mutex.locked {
        return false;
    }
    mutex.locked = true;
    true
}

pub fn mutex_unlock(mutex: &mut KernelMutex) -> bool {
    if !mutex.locked {
        return false;
    }
    if mutex.waiters > 0 {
        mutex.waiters -= 1;
        mutex.locked = true;
    } else {
        mutex.locked = false;
    }
    true
}

pub fn mutex_waiters(mutex: &KernelMutex) -> u32 {
    mutex.waiters
}

pub fn mutex_is_locked(mutex: &KernelMutex) -> bool {
    mutex.locked
}

pub fn sem_init(sem: &mut KernelSemaphore, initial: i32) -> bool {
    if initial < 0 {
        return false;
    }
    sem.value = initial;
    sem.waiters = 0;
    true
}

pub fn sem_wait(sem: &mut KernelSemaphore) -> bool {
    if sem.value > 0 {
        sem.value -= 1;
        return true;
    }
    sem.waiters += 1;
    false
}

pub fn sem_post(sem: &mut KernelSemaphore) -> bool {
    if sem.waiters > 0 {
        sem.waiters -= 1;
        return true;
    }
    sem.value += 1;
    false
}

pub fn sem_value(sem: &KernelSemaphore) -> i32 {
    sem.value
}

pub fn sem_waiters(sem: &KernelSemaphore) -> u32 {
    sem.waiters
}

pub fn proc_fork(parent_pid: u32, child_pid: u32) -> bool {
    if child_pid == 0 {
        return false;
    }

    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let parent_idx = match proc_find(state, parent_pid) {
            Some(i) => i,
            None => return false,
        };
        if proc_find(state, child_pid).is_some() {
            return false;
        }
        let child_idx = match proc_free_slot(state) {
            Some(s) => s,
            None => return false,
        };

        state.used[child_idx] = 1;
        state.exited[child_idx] = 0;
        state.exit_code[child_idx] = 0;
        state.pids[child_idx] = child_pid;
        state.ppids[child_idx] = parent_pid;
        state.cwd[child_idx] = state.cwd[parent_idx];
        state.cwd_len[child_idx] = state.cwd_len[parent_idx];
        proc_name_init_idx(state, child_idx, child_pid);
        state.sig_handlers[child_idx] = state.sig_handlers[parent_idx];
        state.sig_blocked[child_idx] = state.sig_blocked[parent_idx];
        state.sig_pending[child_idx] = 0;
        state.sig_altstack_sp[child_idx] = state.sig_altstack_sp[parent_idx];
        state.sig_altstack_size[child_idx] = state.sig_altstack_size[parent_idx];
        state.sig_altstack_flags[child_idx] = state.sig_altstack_flags[parent_idx];
        state.exec_entry[child_idx] = state.exec_entry[parent_idx];
        state.exec_phoff[child_idx] = state.exec_phoff[parent_idx];
        state.exec_phentsize[child_idx] = state.exec_phentsize[parent_idx];
        state.exec_phnum[child_idx] = state.exec_phnum[parent_idx];
        state.exec_load_count[child_idx] = state.exec_load_count[parent_idx];
        state.exec_has_interp[child_idx] = state.exec_has_interp[parent_idx];
        state.exec_interp_off[child_idx] = state.exec_interp_off[parent_idx];
        state.exec_interp_len[child_idx] = state.exec_interp_len[parent_idx];
        state.count += 1;
        true
    })();
    proc_unlock();
    ok
}

pub fn proc_exit(pid: u32, code: i32) -> bool {
    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return false,
        };
        if state.exited[idx] != 0 {
            return false;
        }
        state.exited[idx] = 1;
        state.exit_code[idx] = code;
        true
    })();
    proc_unlock();
    ok
}

fn proc_wait_inner(state: &mut ProcState, parent_pid: u32, pid: i32, options: u32) -> (i32, i32, bool) {
    let _parent_idx = match proc_find(state, parent_pid) {
        Some(i) => i,
        None => return (-1, 0, false),
    };

    let match_idx = if pid == -1 {
        let mut i = 0usize;
        let mut found: Option<usize> = None;
        while i < QOS_PROC_MAX {
            if state.used[i] != 0 && state.ppids[i] == parent_pid && state.exited[i] != 0 {
                found = Some(i);
                break;
            }
            i += 1;
        }
        found
    } else {
        let child_idx = match proc_find(state, pid as u32) {
            Some(i) => i,
            None => {
                return if (options & QOS_WAIT_WNOHANG) != 0 {
                    (0, 0, false)
                } else {
                    (-1, 0, false)
                }
            }
        };
        if state.ppids[child_idx] != parent_pid {
            return (-1, 0, false);
        }
        if state.exited[child_idx] == 0 {
            return if (options & QOS_WAIT_WNOHANG) != 0 {
                (0, 0, false)
            } else {
                (-1, 0, false)
            };
        }
        Some(child_idx)
    };

    let idx = match match_idx {
        Some(i) => i,
        None => {
            return if (options & QOS_WAIT_WNOHANG) != 0 {
                (0, 0, false)
            } else {
                (-1, 0, false)
            }
        }
    };

    let child_pid = state.pids[idx] as i32;
    let status = state.exit_code[idx];
    state.used[idx] = 0;
    state.exited[idx] = 0;
    state.exit_code[idx] = 0;
    state.pids[idx] = 0;
    state.ppids[idx] = 0;
    state.cwd[idx] = [0; QOS_VFS_PATH_MAX];
    state.cwd_len[idx] = 0;
    state.names[idx] = [0; QOS_PROC_NAME_MAX];
    state.name_len[idx] = 0;
    state.sig_handlers[idx] = [QOS_SIG_DFL; QOS_SIGNAL_MAX];
    state.sig_pending[idx] = 0;
    state.sig_blocked[idx] = 0;
    state.sig_altstack_sp[idx] = 0;
    state.sig_altstack_size[idx] = 0;
    state.sig_altstack_flags[idx] = 0;
    proc_exec_image_clear_idx(state, idx);
    state.count -= 1;
    (child_pid, status, true)
}

pub fn proc_wait(parent_pid: u32, pid: i32, options: u32) -> i32 {
    proc_lock();
    let result = {
        let state = proc_state_mut();
        let (child_pid, _, _) = proc_wait_inner(state, parent_pid, pid, options);
        child_pid
    };
    proc_unlock();
    result
}

pub fn proc_wait_status(parent_pid: u32, pid: i32, options: u32) -> (i32, i32, bool) {
    proc_lock();
    let result = {
        let state = proc_state_mut();
        proc_wait_inner(state, parent_pid, pid, options)
    };
    proc_unlock();
    result
}

pub fn proc_cwd_set(pid: u32, path: &str) -> bool {
    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return false,
        };
        proc_cwd_set_idx(state, idx, path)
    })();
    proc_unlock();
    ok
}

pub fn proc_cwd_len(pid: u32) -> Option<usize> {
    proc_lock();
    let len = (|| {
        let state = proc_state();
        proc_find(state, pid).map(|idx| state.cwd_len[idx] as usize)
    })();
    proc_unlock();
    len
}

fn proc_cwd_snapshot(pid: u32) -> Option<([u8; QOS_VFS_PATH_MAX], usize)> {
    proc_lock();
    let snapshot = (|| {
        let state = proc_state();
        proc_find(state, pid).map(|idx| (state.cwd[idx], state.cwd_len[idx] as usize))
    })();
    proc_unlock();
    snapshot
}

pub fn proc_exec_signal_reset(pid: u32) -> bool {
    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return false,
        };

        let mut s = 1usize;
        while s < QOS_SIGNAL_MAX {
            if state.sig_handlers[idx][s] != QOS_SIG_IGN {
                state.sig_handlers[idx][s] = QOS_SIG_DFL;
            }
            s += 1;
        }
        state.sig_pending[idx] = 0;
        state.sig_blocked[idx] &= !proc_signal_unmaskable_bits();
        true
    })();
    proc_unlock();
    ok
}

pub fn proc_exec_image_set(pid: u32, image: ProcExecImage) -> bool {
    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return false,
        };
        state.exec_entry[idx] = image.entry;
        state.exec_phoff[idx] = image.phoff;
        state.exec_phentsize[idx] = image.phentsize;
        state.exec_phnum[idx] = image.phnum;
        state.exec_load_count[idx] = image.load_count;
        state.exec_has_interp[idx] = if image.has_interp { 1 } else { 0 };
        state.exec_interp_off[idx] = image.interp_off;
        state.exec_interp_len[idx] = image.interp_len;
        true
    })();
    proc_unlock();
    ok
}

pub fn proc_exec_image_get(pid: u32) -> Option<ProcExecImage> {
    proc_lock();
    let img = (|| {
        let state = proc_state();
        let idx = proc_find(state, pid)?;
        Some(ProcExecImage {
            entry: state.exec_entry[idx],
            phoff: state.exec_phoff[idx],
            phentsize: state.exec_phentsize[idx],
            phnum: state.exec_phnum[idx],
            load_count: state.exec_load_count[idx],
            has_interp: state.exec_has_interp[idx] != 0,
            interp_off: state.exec_interp_off[idx],
            interp_len: state.exec_interp_len[idx],
        })
    })();
    proc_unlock();
    img
}

pub fn proc_signal_set_handler(pid: u32, signum: u32, handler: u32) -> bool {
    if !proc_signal_signum_valid(signum) {
        return false;
    }
    if (signum == QOS_SIGKILL || signum == QOS_SIGSTOP) && handler != QOS_SIG_DFL {
        return false;
    }

    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return false,
        };
        state.sig_handlers[idx][signum as usize] = handler;
        true
    })();
    proc_unlock();
    ok
}

pub fn proc_signal_get_handler(pid: u32, signum: u32) -> Option<u32> {
    if !proc_signal_signum_valid(signum) {
        return None;
    }

    proc_lock();
    let handler = (|| {
        let state = proc_state();
        proc_find(state, pid).map(|idx| state.sig_handlers[idx][signum as usize])
    })();
    proc_unlock();
    handler
}

pub fn proc_signal_set_mask(pid: u32, mask: u32) -> bool {
    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return false,
        };
        state.sig_blocked[idx] = mask & !proc_signal_unmaskable_bits();
        true
    })();
    proc_unlock();
    ok
}

pub fn proc_signal_mask(pid: u32) -> Option<u32> {
    proc_lock();
    let mask = (|| {
        let state = proc_state();
        proc_find(state, pid).map(|idx| state.sig_blocked[idx])
    })();
    proc_unlock();
    mask
}

pub fn proc_signal_pending(pid: u32) -> Option<u32> {
    proc_lock();
    let pending = (|| {
        let state = proc_state();
        proc_find(state, pid).map(|idx| state.sig_pending[idx])
    })();
    proc_unlock();
    pending
}

pub fn proc_signal_send(pid: u32, signum: u32) -> bool {
    if !proc_signal_signum_valid(signum) {
        return false;
    }

    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return false,
        };
        state.sig_pending[idx] |= proc_signal_bit(signum);
        true
    })();
    proc_unlock();
    ok
}

pub fn proc_signal_next(pid: u32) -> i32 {
    proc_lock();
    let signum = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return -1,
        };
        let pending = state.sig_pending[idx];
        if pending == 0 {
            return 0;
        }
        let deliverable = (pending & !state.sig_blocked[idx]) | (pending & proc_signal_unmaskable_bits());
        if deliverable == 0 {
            return 0;
        }

        let mut s = 1u32;
        while (s as usize) < QOS_SIGNAL_MAX {
            let bit = proc_signal_bit(s);
            if (deliverable & bit) != 0 {
                state.sig_pending[idx] &= !bit;
                return s as i32;
            }
            s += 1;
        }
        0
    })();
    proc_unlock();
    signum
}

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct SignalAltStack {
    pub sp: u64,
    pub size: u64,
    pub flags: u32,
}

pub fn proc_signal_set_altstack(pid: u32, sp: u64, size: u64, flags: u32) -> bool {
    proc_lock();
    let ok = (|| {
        let state = proc_state_mut();
        let idx = match proc_find(state, pid) {
            Some(i) => i,
            None => return false,
        };
        state.sig_altstack_sp[idx] = sp;
        state.sig_altstack_size[idx] = size;
        state.sig_altstack_flags[idx] = flags;
        true
    })();
    proc_unlock();
    ok
}

pub fn proc_signal_get_altstack(pid: u32) -> Option<SignalAltStack> {
    proc_lock();
    let alt = (|| {
        let state = proc_state();
        proc_find(state, pid).map(|idx| SignalAltStack {
            sp: state.sig_altstack_sp[idx],
            size: state.sig_altstack_size[idx],
            flags: state.sig_altstack_flags[idx],
        })
    })();
    proc_unlock();
    alt
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct KernelStatus {
    pub init_state: u32,
    pub pmm_total_pages: u32,
    pub pmm_free_pages: u32,
    pub sched_count: u32,
    pub vfs_count: u32,
    pub net_queue_len: u32,
    pub drivers_count: u32,
    pub syscall_count: u32,
    pub proc_count: u32,
}

pub fn kernel_status() -> KernelStatus {
    KernelStatus {
        init_state: init_state(),
        pmm_total_pages: pmm_total_pages(),
        pmm_free_pages: pmm_free_pages(),
        sched_count: sched_count(),
        vfs_count: vfs_count(),
        net_queue_len: net_queue_len(),
        drivers_count: drivers_count(),
        syscall_count: syscall_count(),
        proc_count: proc_count(),
    }
}

pub fn kernel_entry(boot_info: &BootInfo) -> Result<(), KernelError> {
    reset_init_state();
    vmm_reset();

    if !boot_info_valid(boot_info) {
        return Err(KernelError::InvalidBootInfo);
    }

    if pmm_init_from_boot_info(boot_info).is_err() {
        return Err(KernelError::InvalidBootInfo);
    }

    mm_init();
    drivers_init();
    vfs_init();
    net_init();
    softirq_init();
    timer_init();
    napi_init();
    kthread_init();
    workqueue_init();
    sched_init();
    syscall_init();
    proc_init();

    if init_state() != QOS_INIT_ALL {
        return Err(KernelError::InitIncomplete);
    }

    Ok(())
}
