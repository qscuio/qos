pub const QOS_BOOT_MAGIC: u64 = 0x514F53424F4F5400;
pub const QOS_MMAP_MAX_ENTRIES: usize = 128;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct MmapEntry {
    pub base: u64,
    pub length: u64,
    pub type_: u32,
    pub _pad: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct BootInfo {
    pub magic: u64,

    pub mmap_entry_count: u32,
    pub _pad0: u32,
    pub mmap_entries: [MmapEntry; QOS_MMAP_MAX_ENTRIES],

    pub fb_addr: u64,
    pub fb_width: u32,
    pub fb_height: u32,
    pub fb_pitch: u32,
    pub fb_bpp: u8,
    pub _pad1: [u8; 3],

    pub initramfs_addr: u64,
    pub initramfs_size: u64,

    pub acpi_rsdp_addr: u64,
    pub dtb_addr: u64,
}

impl Default for BootInfo {
    fn default() -> Self {
        Self {
            magic: QOS_BOOT_MAGIC,
            mmap_entry_count: 0,
            _pad0: 0,
            mmap_entries: [MmapEntry::default(); QOS_MMAP_MAX_ENTRIES],
            fb_addr: 0,
            fb_width: 0,
            fb_height: 0,
            fb_pitch: 0,
            fb_bpp: 0,
            _pad1: [0; 3],
            initramfs_addr: 0,
            initramfs_size: 0,
            acpi_rsdp_addr: 0,
            dtb_addr: 0,
        }
    }
}
