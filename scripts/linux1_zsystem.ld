ENTRY(startup_32)

SECTIONS
{
  . = 0x00100000;

  .text : {
    boot/head.o(.text)
    *(.text.unlikely .text.*_unlikely .text.unlikely.*)
    *(.text.exit .text.exit.*)
    *(.text.startup .text.startup.*)
    *(.text.hot .text.hot.*)
    *(SORT_BY_NAME(.text.sorted.*))
    *(.text .stub .text.* .gnu.linkonce.t.*)
  }

  _etext = .;

  .rodata ALIGN(32) : {
    *(.rodata .rodata.*)
  }

  .eh_frame ALIGN(4) : {
    *(.eh_frame)
  }

  .data ALIGN(32) : {
    *(.data .data.*)
  }

  _edata = .;

  .bss ALIGN(32) : {
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
  }

  _end = .;

  /DISCARD/ : {
    *(.comment)
    *(.note.GNU-stack)
  }
}
