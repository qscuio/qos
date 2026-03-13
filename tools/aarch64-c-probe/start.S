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

.section .text.vectors, "ax"
.balign 2048
.global qos_vector_table
qos_vector_table:
.macro VECTOR_BRANCH target
    b   \target
    .space 124
.endm
    VECTOR_BRANCH qos_vector_default    // EL1t sync
    VECTOR_BRANCH qos_vector_default    // EL1t irq
    VECTOR_BRANCH qos_vector_default    // EL1t fiq
    VECTOR_BRANCH qos_vector_default    // EL1t serr
    VECTOR_BRANCH qos_vector_default    // EL1h sync
    VECTOR_BRANCH qos_vector_irq_el1h   // EL1h irq
    VECTOR_BRANCH qos_vector_default    // EL1h fiq
    VECTOR_BRANCH qos_vector_default    // EL1h serr
    VECTOR_BRANCH qos_vector_default    // EL0 64 sync
    VECTOR_BRANCH qos_vector_default    // EL0 64 irq
    VECTOR_BRANCH qos_vector_default    // EL0 64 fiq
    VECTOR_BRANCH qos_vector_default    // EL0 64 serr
    VECTOR_BRANCH qos_vector_default    // EL0 32 sync
    VECTOR_BRANCH qos_vector_default    // EL0 32 irq
    VECTOR_BRANCH qos_vector_default    // EL0 32 fiq
    VECTOR_BRANCH qos_vector_default    // EL0 32 serr

qos_vector_irq_el1h:
    sub  sp, sp, #160
    stp  x0, x1, [sp, #0]
    stp  x2, x3, [sp, #16]
    stp  x4, x5, [sp, #32]
    stp  x6, x7, [sp, #48]
    stp  x8, x9, [sp, #64]
    stp  x10, x11, [sp, #80]
    stp  x12, x13, [sp, #96]
    stp  x14, x15, [sp, #112]
    stp  x16, x17, [sp, #128]
    str  x18, [sp, #144]
    str  x30, [sp, #152]
    bl   qos_irq_dispatch
    ldr  x30, [sp, #152]
    ldr  x18, [sp, #144]
    ldp  x16, x17, [sp, #128]
    ldp  x14, x15, [sp, #112]
    ldp  x12, x13, [sp, #96]
    ldp  x10, x11, [sp, #80]
    ldp  x8, x9, [sp, #64]
    ldp  x6, x7, [sp, #48]
    ldp  x4, x5, [sp, #32]
    ldp  x2, x3, [sp, #16]
    ldp  x0, x1, [sp, #0]
    add  sp, sp, #160
    eret

qos_vector_default:
    wfe
    b    qos_vector_default

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
