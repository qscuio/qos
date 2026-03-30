        AREA    |.drectve|, DRECTVE

        EXPORT  |funchook_hook_caller_asm|
        IMPORT  |funchook_hook_caller|

        AREA    |.text$mn|, CODE, ARM
|funchook_hook_caller_asm| PROC
        // save frame pointer (r7) and link register (lr).
        stmfd sp!, {r7, lr}
        // set frame pointer
        add r7, sp, #0
        // save integer or pointer arguments passed in registers.
        stmfd sp!, {r0-r3}
        // save floating-point registers used as arguments (s0-s15).
        vst1.32 {d0-d7}, [sp:64]!
        // 1st arg: the start address of transit. Note: r10 is set by transit-aarch64.s.
        mov r0, r10
        // 2nd arg: frame pointer
        mov r1, r7
        // call funchook_hook_caller
        bl  funchook_hook_caller
        mov r9, r0
        // restore registers
        vld1.32 {d0-d7}, [sp:64]
        ldmfd sp!, {r0-r3}
        ldmfd sp!, {r7, lr}
        // jump to hook_func
        bx r9
        ENDP
        END

