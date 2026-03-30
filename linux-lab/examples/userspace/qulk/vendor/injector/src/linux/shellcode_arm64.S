#if defined(__aarch64__)
#define handle_offset 0
#define dlopen_addr_offset 4
#define dlerror_addr_offset 8
#define dlflags_offset 12
#define file_path_offset 16
#define page_size 4096

		// .text specifies that the following code is in the text (code) section.
		// .global makes injector_shellcode globally visible.
		// .hidden hides the symbol from the dynamic symbol table.
		// .type defines injector_shellcode as a function.

        .text
        .global injector_shellcode
        .hidden injector_shellcode
        .type   injector_shellcode, %function

        // void *injector_shellcode(injector_shellcode_arg_t *arg) {
injector_shellcode:
        // prolog
		// The stp instruction stores the frame pointer (x29) and the return address (x30) on the stack.
        stp     x29, x30, [sp, #-16]!    // Store frame pointer and return address
        mov     x29, sp                 // Set up frame pointer

        // int dlflags = arg->dlflags;
        ldr     w1, [x0, #dlflags_offset]

        // const char *file_path = arg->file_path;
        add     x1, x0, #file_path_offset

        // void *handle = dlopen(file_path, dlflags);
        ldr     x2, [x0, #dlopen_addr_offset]
        blr     x2

        // arg->handle = handle;
        str     x0, [x0, #handle_offset]

        // arg->file_path[0] = '\0';
        mov     w2, #0
        strb    w2, [x0, #file_path_offset]

        // if (handle != NULL) return;
        cbnz    x0, .exit

        // if (arg->dlerror_addr == 0) return;
        ldr     x2, [x0, #dlerror_addr_offset]
        cbz     x2, .exit

        // char *errmsg = dlerror();
        blr     x2

        // if (errmsg == NULL) return;
        cbz     x0, .exit

        // char *dest = arg->file_path;
        add     x1, x0, #file_path_offset

        // char *end = (char*)arg + page_size;
        add     x2, x0, #page_size

.loop:
        // char c = *(errmsg++);
        ldrb    w3, [x0], #1

        // *(dest++) = c;
        strb    w3, [x1], #1

        // if (c == 0) return;
        cbz     w3, .exit

        // if (dest < end) goto loop;
        cmp     x1, x2
        b.lo    .loop

.exit:
        // epilog
        ldp     x29, x30, [sp], #16      // Restore frame pointer and return address
        ret
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

