ENTRY(_start)

KERNEL_VIRT_BASE = 0xFFFFFFFF80000000;
KERNEL_PHYS_BASE = 0x40080000;
KERNEL_DELTA = KERNEL_VIRT_BASE - KERNEL_PHYS_BASE;

SECTIONS
{
  . = KERNEL_VIRT_BASE;

  .text : AT(ADDR(.text) - KERNEL_DELTA) {
    KEEP(*(.text._start))
    *(.text .text.*)
  }

  .rodata : AT(ADDR(.rodata) - KERNEL_DELTA) {
    *(.rodata .rodata.*)
  }

  .data : AT(ADDR(.data) - KERNEL_DELTA) {
    *(.data .data.*)
  }

  .bss (NOLOAD) : AT(ADDR(.bss) - KERNEL_DELTA) {
    *(.bss .bss.*)
    *(COMMON)
  }

  __kernel_phys_start = LOADADDR(.text);
  __kernel_phys_end = LOADADDR(.bss) + SIZEOF(.bss);
}
