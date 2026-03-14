.section .rodata
prompt:
    .ascii "linux1-sh# "
prompt_len = . - prompt
exit_word:
    .ascii "exit\n"

.section .bss
    .lcomm inbuf, 128

.section .text
.globl _start
.include "common.inc"

_start:
shell_loop:
    # print prompt
    movl $SYS_WRITE, %eax
    movl $1, %ebx
    movl $prompt, %ecx
    movl $prompt_len, %edx
    int $0x80

    # read line
    movl $SYS_READ, %eax
    xorl %ebx, %ebx
    movl $inbuf, %ecx
    movl $128, %edx
    int $0x80
    cmpl $0, %eax
    jle shell_exit

    # if buffer starts with "exit\n", exit to let init respawn
    movb inbuf, %bl
    cmpb $'e', %bl
    jne shell_loop
    movb inbuf+1, %bl
    cmpb $'x', %bl
    jne shell_loop
    movb inbuf+2, %bl
    cmpb $'i', %bl
    jne shell_loop
    movb inbuf+3, %bl
    cmpb $'t', %bl
    jne shell_loop

shell_exit:
    movl $SYS_EXIT, %eax
    xorl %ebx, %ebx
    int $0x80
