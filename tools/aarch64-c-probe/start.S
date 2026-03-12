.section .text._start, "ax"
.global _start
_start:
    adrp x1, QOS_STACK
    add  x1, x1, :lo12:QOS_STACK
    mov  x2, #65536
    add  x1, x1, x2
    mov  sp, x1
    bl   c_main
1:
    wfe
    b    1b
