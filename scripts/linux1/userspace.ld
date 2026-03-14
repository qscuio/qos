ENTRY(_start)

PHDRS
{
  all PT_LOAD FLAGS(7);
}

SECTIONS
{
  . = 0x08048000 + SIZEOF_HEADERS;

  .text : {
    *(.text*)
  } :all

  .rodata : {
    *(.rodata*)
  } :all

  .data : {
    *(.data*)
    *(.bss*)
    *(COMMON)
    . = ALIGN(0x1000);
  } :all
}
