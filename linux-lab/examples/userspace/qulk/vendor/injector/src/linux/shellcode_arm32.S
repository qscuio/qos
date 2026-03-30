#if defined(__arm__)
#define handle_offset 0
#define dlopen_addr_offset 4
#define dlerror_addr_offset 8
#define dlflags_offset 12
#define file_path_offset 16
#define page_size 4096

        .text
        .global injector_shellcode
        .hidden injector_shellcode
        .type   injector_shellcode, %function

        // void *injector_shellcode(injector_shellcode_arg_t *arg) {
injector_shellcode:
        // prolog
        push    {lr, fp}                // Save link register and frame pointer
        add     fp, sp, #4             // Set up frame pointer

        // int dlflags = arg->dlflags;
        ldr     r1, [r0, #dlflags_offset]

        // const char *file_path = arg->file_path;
        add     r1, r0, #file_path_offset

        // void *handle = dlopen(file_path, dlflags);
        ldr     r2, [r0, #dlopen_addr_offset]
        blx     r2

        // arg->handle = handle;
        str     r0, [r0, #handle_offset]

        // arg->file_path[0] = '\0';
        mov     r2, #0
        strb    r2, [r0, #file_path_offset]

        // if (handle != NULL) return;
        cmp     r0, #0
        bne     .exit

        // if (arg->dlerror_addr == 0) return;
        ldr     r2, [r0, #dlerror_addr_offset]
        cmp     r2, #0
        beq     .exit

        // char *errmsg = dlerror();
        blx     r2

        // if (errmsg == NULL) return;
        cmp     r0, #0
        beq     .exit

        // char *dest = arg->file_path;
        add     r1, r0, #file_path_offset

        // char *end = (char*)arg + page_size;
        add     r2, r0, #page_size

.loop:
        // char c = *(errmsg++);
        ldrb    r3, [r0], #1

        // *(dest++) = c;
        strb    r3, [r1], #1

        // if (c == 0) return;
        cmp     r3, #0
        beq     .exit

        // if (dest < end) goto loop;
        cmp     r1, r2
        blt     .loop

.exit:
        // epilog
        pop     {fp, lr}                // Restore frame pointer and link register
        bx      lr                      // Return from function
        // }

        .size   injector_shellcode, . - injector_shellcode

        .balign  4
        .global injector_shellcode_size
        .hidden injector_shellcode_size
        .type   injector_shellcode_size, %object
        .size   injector_shellcode_size, 4
injector_shellcode_size:
        // distance from injector_shellcode to current.
        .int    . - injector_shellcode
#endif

