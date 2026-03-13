.section .text
.globl _start
_start:
    movl $1, %eax
    xorl %ebx, %ebx
    int $0x80
