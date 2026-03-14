.section .rodata
msg_init:
    .ascii "linux1-init: start\n"
msg_init_len = . - msg_init

msg_exit:
    .ascii "linux1-init: shell exited\n"
msg_exit_len = . - msg_exit

msg_exec_fail:
    .ascii "linux1-init: exec failed\n"
msg_exec_fail_len = . - msg_exec_fail

shell_path:
    .asciz "/bin/sh"
tty_path:
    .asciz "/dev/tty1"

.section .data
argv:
    .long shell_path
    .long 0
status_word:
    .long 0

.section .text
.globl _start
.include "common.inc"

_start:
    # stay on the kernel's inherited text console (tty1)
    movl $SYS_OPEN, %eax
    movl $tty_path, %ebx
    movl $2, %ecx              # O_RDWR
    xorl %edx, %edx
    int $0x80
    cmpl $0, %eax
    jl no_console_stdio

    movl %eax, %esi

    movl $SYS_CLOSE, %eax
    xorl %ebx, %ebx
    int $0x80
    movl $SYS_CLOSE, %eax
    movl $1, %ebx
    int $0x80
    movl $SYS_CLOSE, %eax
    movl $2, %ebx
    int $0x80

    movl $SYS_DUP, %eax
    movl %esi, %ebx
    int $0x80
    movl $SYS_DUP, %eax
    movl %esi, %ebx
    int $0x80
    movl $SYS_DUP, %eax
    movl %esi, %ebx
    int $0x80

no_console_stdio:
    # write init start marker
    movl $SYS_WRITE, %eax
    movl $1, %ebx
    movl $msg_init, %ecx
    movl $msg_init_len, %edx
    int $0x80

respawn_shell:
    movl $SYS_FORK, %eax
    int $0x80
    cmpl $0, %eax
    jl fatal_exit
    je child_exec

    # parent waits for child exit
    movl %eax, %ebx
    movl $status_word, %ecx
    xorl %edx, %edx
    movl $SYS_WAITPID, %eax
    int $0x80

    # print shell exited marker and respawn
    movl $SYS_WRITE, %eax
    movl $1, %ebx
    movl $msg_exit, %ecx
    movl $msg_exit_len, %edx
    int $0x80
    jmp respawn_shell

child_exec:
    movl $SYS_EXECVE, %eax
    movl $shell_path, %ebx
    movl $argv, %ecx
    xorl %edx, %edx
    int $0x80

    # exec failed in child
    movl $SYS_WRITE, %eax
    movl $1, %ebx
    movl $msg_exec_fail, %ecx
    movl $msg_exec_fail_len, %edx
    int $0x80

fatal_exit:
    movl $SYS_EXIT, %eax
    movl $1, %ebx
    int $0x80
