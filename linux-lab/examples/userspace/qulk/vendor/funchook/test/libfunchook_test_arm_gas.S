#ifdef __APPLE__
#define call_get_val_in_dll _call_get_val_in_dll
#define get_val_in_dll _get_val_in_dll
#define jump_get_val_in_dll _jump_get_val_in_dll
#define arm_test_adr _arm_test_adr
#define arm_test_beq _arm_test_beq
#define arm_test_bne _arm_test_bne
#define arm_test_cbnz _arm_test_cbnz
#define arm_test_cbz _arm_test_cbz
#define arm_test_ldr_w _arm_test_ldr_w
#define arm_test_ldr _arm_test_ldr
#define arm_test_prfm _arm_test_prfm
#define arm_test_ldrsw _arm_test_ldrsw
#define arm_test_tbnz _arm_test_tbnz
#define arm_test_tbz _arm_test_tbz
#define test_data _test_data
#endif

        .arch armv8-a
        .file   "libfunchook_test_arm_gas.S"
        .text
        .align  2
        .p2align 3,,7

test_data:
        .word 0x10203040, 0x50607080, 0x01020304, 0x05060708

call_get_val_in_dll:
        .global call_get_val_in_dll
#ifdef __linux__
        .type   call_get_val_in_dll, %function
#endif
        push    {r4, lr}
        bl      get_val_in_dll
        pop     {r4, lr}
        bx      lr

jump_get_val_in_dll:
        .global jump_get_val_in_dll
#ifdef __linux__
        .type   jump_get_val_in_dll, %function
#endif
        b       get_val_in_dll

arm_test_adr:
        .global arm_test_adr
#ifdef __linux__
        .type   arm_test_adr, %function
#endif
        adr r9, test_data
        ldr r9, [r9]
        add r0, r0, r9
        bx lr

arm_test_beq:
        .global arm_test_beq
#ifdef __linux__
        .type   arm_test_beq, %function
#endif
        adds r0, r0, #1
        beq 1f
        subs r0, r0, #2
1:
        bx lr

arm_test_bne:
        .global arm_test_bne
#ifdef __linux__
        .type   arm_test_bne, %function
#endif
        adds r0, r0, #1
        bne 1f
        subs r0, r0, #2
1:
        bx lr

arm_test_cbnz:
        .global arm_test_cbnz
#ifdef __linux__
        .type   arm_test_cbnz, %function
#endif
        cbnz r0, 1f
        adds r0, r0, #2
1:
        subs r0, r0, #1
        bx lr

arm_test_cbz:
        .global arm_test_cbz
#ifdef __linux__
        .type   arm_test_cbz, %function
#endif
        cbz r0, 1f
        adds r0, r0, #2
1:
        subs r0, r0, #1
        bx lr

arm_test_ldr_w:
        .global arm_test_ldr_w
#ifdef __linux__
        .type   arm_test_ldr_w, %function
#endif
        ldr r9, test_data
        add r0, r0, r9
        bx lr

arm_test_ldr:
        .global arm_test_ldr
#ifdef __linux__
        .type   arm_test_ldr, %function
#endif
        ldr r9, test_data
        add r0, r0, r9
        bx lr

arm_test_prfm:
        .global arm_test_prfm
#ifdef __linux__
        .type   arm_test_prfm, %function
#endif
        ldr r9, test_data
        add r0, r0, r9
        bx lr

arm_test_ldrsw:
        .global arm_test_ldrsw
#ifdef __linux__
        .type   arm_test_ldrsw, %function
#endif
        ldr r9, =1f
        ldr r9, [r9]
        add r0, r0, r9
        bx lr

arm_test_tbnz:
        .global arm_test_tbnz
#ifdef __linux__
        .type   arm_test_tbnz, %function
#endif
        tbnz r0, #32, 1f
        adds r0, r0, #2
1:
        subs r0, r0, #1
        bx lr

arm_test_tbz:
        .global arm_test_tbz
#ifdef __linux__
        .type   arm_test_tbz, %function
#endif
        tbz r0, #32, 1f
        adds r0, r0, #2
1:
        subs r0, r0, #1
        bx lr

