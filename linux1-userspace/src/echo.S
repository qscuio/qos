.section .rodata
nl:
    .ascii "\n"

.section .text
.globl _start
.include "common.inc"

_start:
    # Minimal echo placeholder for initial milestone: newline only.
    movl $SYS_WRITE, %eax
    movl $1, %ebx
    movl $nl, %ecx
    movl $1, %edx
    int $0x80

    movl $SYS_EXIT, %eax
    xorl %ebx, %ebx
    int $0x80
