#if defined __APPLE__
#define funchook_hook_caller_asm _funchook_hook_caller_asm
#define funchook_hook_caller _funchook_hook_caller
#endif
        .arch armv8-a
        .text
        .globl  funchook_hook_caller_asm // 将 funchook_hook_caller_asm 声明为一个全局符号，可以在其他文件中引用。
#if defined __linux__
        .type   funchook_hook_caller_asm, %function
#endif
funchook_hook_caller_asm:
        .cfi_startproc
        // save frame pointer (r7) and link register (lr).
        stmfd sp!, {r7, lr}
        .cfi_def_cfa_offset 8
        .cfi_offset 7, -8
        .cfi_offset 14, -4
        // set frame pointer
        add r7, sp, #0
        // save integer or pointer arguments passed in registers.
        stmfd sp!, {r0-r3}
        .cfi_offset 0, -12
        .cfi_offset 1, -16
        .cfi_offset 2, -20
        .cfi_offset 3, -24
        // save floating-point registers used as arguments (s0-s15).
        vst1.32 {d0-d7}, [sp:64]!
        .cfi_offset d0, -32
        .cfi_offset d1, -40
        .cfi_offset d2, -48
        .cfi_offset d3, -56
        .cfi_offset d4, -64
        .cfi_offset d5, -72
        .cfi_offset d6, -80
        .cfi_offset d7, -88
        // 1st arg: the start address of transit. Note: r10 is set by transit-aarch64.s.
        mov r0, r10
        // 2nd arg: frame pointer
        mov r1, r7
        // call funchook_hook_caller
        bl  funchook_hook_caller
        mov r9, r0
        // restore registers
        vld1.32 {d0-d7}, [sp:64]
        .cfi_restore d0
        .cfi_restore d1
        .cfi_restore d2
        .cfi_restore d3
        .cfi_restore d4
        .cfi_restore d5
        .cfi_restore d6
        .cfi_restore d7
        ldmfd sp!, {r0-r3}
        .cfi_restore 0
        .cfi_restore 1
        .cfi_restore 2
        .cfi_restore 3
        ldmfd sp!, {r7, lr}
        .cfi_restore 7
        .cfi_restore 14
        .cfi_def_cfa_offset 0
        // jump to hook_func
        bx r9
        .cfi_endproc

