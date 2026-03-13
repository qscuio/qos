.section .text._start, "ax"
.global _start
_start:
    mov  x19, x0
    mrs  x20, CurrentEL
    lsr  x20, x20, #2
    cmp  x20, #2
    b.ne 1f
    mov  x20, #3
    msr  cnthctl_el2, x20
    msr  cntvoff_el2, xzr
    mov  x20, #(1 << 31)
    msr  hcr_el2, x20
    mov  x20, #0x3c5
    msr  spsr_el2, x20
    adr  x20, 1f
    msr  elr_el2, x20
    isb
    eret
1:
    mov  x0, x19
    adrp x1, QOS_STACK
    add  x1, x1, :lo12:QOS_STACK
    mov  x2, #65536
    add  x1, x1, x2
    mov  sp, x1
    bl   c_main
2:
    wfe
    b    2b
