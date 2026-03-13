ENTRY(_start)

SECTIONS
{
  . = 0x40080000;
  __kernel_phys_start = .;

  .text : ALIGN(4K) {
    KEEP(*(.text._start))
    *(.text .text.*)
  }

  .rodata : ALIGN(4K) {
    *(.rodata .rodata.*)
  }

  .data : ALIGN(4K) {
    *(.data .data.*)
  }

  .bss (NOLOAD) : ALIGN(4K) {
    *(.bss .bss.*)
    *(COMMON)
  }

  __kernel_phys_end = .;
}
