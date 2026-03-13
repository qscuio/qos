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
    mrs  x20, cpacr_el1
    orr  x20, x20, #(3 << 20)
    msr  cpacr_el1, x20
    isb

    mrs  x20, sctlr_el1
    tbnz x20, #0, 2f

    adrp x21, qos_ttbr0_l0_table
    add  x21, x21, :lo12:qos_ttbr0_l0_table
    adrp x22, qos_ttbr1_l0_table
    add  x22, x22, :lo12:qos_ttbr1_l0_table
    adrp x25, qos_ttbr0_l1_table
    add  x25, x25, :lo12:qos_ttbr0_l1_table
    adrp x26, qos_ttbr1_l1_table
    add  x26, x26, :lo12:qos_ttbr1_l1_table

    orr  x23, x25, #0x3
    str  x23, [x21]
    orr  x23, x26, #0x3
    str  x23, [x22, #4088]

    ldr  x24, =0x0060000000000401
    str  x24, [x25]
    ldr  x24, =0x0000000040000705
    str  x24, [x25, #8]
    str  x24, [x26, #4080]
    ldr  x24, =0x0060000000000401
    str  x24, [x26, #4088]

    ldr  x24, =0x000000000000ff00
    msr  mair_el1, x24
    msr  ttbr0_el1, x21
    msr  ttbr1_el1, x22
    ldr  x24, =0x00000005b5103510
    msr  tcr_el1, x24
    dsb  ish
    isb

    mrs  x24, sctlr_el1
    orr  x24, x24, #(1 << 0)
    msr  sctlr_el1, x24
    isb
    ldr  x24, =0xFFFFFFFF40000000
    adr  x25, 2f
    add  x25, x25, x24
    br   x25
2:
    mov  x0, x19
    adrp x1, QOS_STACK
    add  x1, x1, :lo12:QOS_STACK
    mov  x2, #65536
    add  x1, x1, x2
    mov  sp, x1
    bl   c_main
3:
    wfe
    b    3b

.section .bss, "aw", %nobits
.balign 4096
qos_ttbr0_l0_table:
    .space 4096
.balign 4096
qos_ttbr1_l0_table:
    .space 4096
.balign 4096
qos_ttbr0_l1_table:
    .space 4096
.balign 4096
qos_ttbr1_l1_table:
    .space 4096
