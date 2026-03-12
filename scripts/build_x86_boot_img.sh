#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <impl> <output-disk-img>" >&2
  exit 2
fi

impl="$1"
out_img="$2"
out_dir="$(dirname "$out_img")"
mkdir -p "$out_dir"
root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmd_bin_dir="$out_dir/rootfs/bin"
"$root_dir/scripts/build_x86_cmd_bins.sh" "$cmd_bin_dir"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

stage1_s="$tmp_dir/stage1.s"
stage1_o="$tmp_dir/stage1.o"
stage1_elf="$tmp_dir/stage1.elf"
stage1_bin="$tmp_dir/stage1.bin"

stage2_s="$tmp_dir/stage2.s"
stage2_o="$tmp_dir/stage2.o"
stage2_elf="$tmp_dir/stage2.elf"
stage2_bin="$tmp_dir/stage2.bin"

cat > "$stage2_s" <<ASM
.code16
.section .text
.global _start
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    call init_serial

    movw \$0x7000, %bx
    movw %bx, %di
    xorw %ax, %ax
    movw \$1600, %cx
    rep stosw
    movw \$0x5400, 0(%bx)
    movw \$0x4F4F, 2(%bx)
    movw \$0x5342, 4(%bx)
    movw \$0x514F, 6(%bx)
    pushw %bx
    call kernel_main
    addw \$2, %sp

hang:
    hlt
    jmp hang

kernel_main:
    pushw %bp
    movw %sp, %bp

    movw 4(%bp), %bx
    call load_kernel_elf_header
    jc bad_handoff
    call load_initramfs_header
    jc bad_handoff

    movw \$0x7E00, %si
    movw (%si), %ax
    movw %ax, 8(%bx)
    movw \$0x0000, 10(%bx)
    movw 2(%si), %ax
    movw %ax, 16(%bx)
    movw 4(%si), %ax
    movw %ax, 18(%bx)
    movw 6(%si), %ax
    movw %ax, 20(%bx)
    movw 8(%si), %ax
    movw %ax, 22(%bx)
    movw 10(%si), %ax
    movw %ax, 24(%bx)
    movw 12(%si), %ax
    movw %ax, 26(%bx)
    movw 14(%si), %ax
    movw %ax, 28(%bx)
    movw 16(%si), %ax
    movw %ax, 30(%bx)
    movw 18(%si), %ax
    movw %ax, 32(%bx)
    movw 20(%si), %ax
    movw %ax, 34(%bx)
    movw \$0x0000, 36(%bx)
    movw \$0x0000, 38(%bx)
    movw \$0xB000, 3112(%bx)
    movw \$0x0000, 3114(%bx)
    movw \$0x0000, 3116(%bx)
    movw \$0x0000, 3118(%bx)
    movw \$0x0200, 3120(%bx)
    movw \$0x0000, 3122(%bx)
    movw \$0x0000, 3124(%bx)
    movw \$0x0000, 3126(%bx)

    movw 0(%bx), %ax
    cmpw \$0x5400, %ax
    jne bad_handoff
    movw 2(%bx), %ax
    cmpw \$0x4F4F, %ax
    jne bad_handoff
    movw 4(%bx), %ax
    cmpw \$0x5342, %ax
    jne bad_handoff
    movw 6(%bx), %ax
    cmpw \$0x514F, %ax
    jne bad_handoff

    movw 8(%bx), %ax
    cmpw \$0x0001, %ax
    jb bad_handoff
    cmpw \$0x0080, %ax
    ja bad_handoff
    movw 10(%bx), %ax
    cmpw \$0x0000, %ax
    jne bad_handoff
    movw 32(%bx), %ax
    cmpw \$0x0001, %ax
    jne bad_handoff
    movw 34(%bx), %ax
    cmpw \$0x0000, %ax
    jne bad_handoff
    movw 24(%bx), %ax
    orw 26(%bx), %ax
    orw 28(%bx), %ax
    orw 30(%bx), %ax
    jz bad_handoff
    movw \$0x7E20, %si
    movw (%si), %ax
    cmpw \$0x0001, %ax
    je disk_method_ok
    cmpw \$0x0002, %ax
    jne bad_handoff
disk_method_ok:

    movw 3112(%bx), %ax
    orw 3114(%bx), %ax
    orw 3116(%bx), %ax
    orw 3118(%bx), %ax
    jz bad_handoff

    movw 3120(%bx), %ax
    orw 3122(%bx), %ax
    orw 3124(%bx), %ax
    orw 3126(%bx), %ax
    jz bad_handoff

    movw 3128(%bx), %ax
    orw 3130(%bx), %ax
    orw 3132(%bx), %ax
    orw 3134(%bx), %ax
    jne bad_handoff

    movw 3136(%bx), %ax
    orw 3138(%bx), %ax
    orw 3140(%bx), %ax
    orw 3142(%bx), %ax
    jne bad_handoff

    movw \$msg_ok_prefix, %si
    call print_string
    movw 8(%bx), %ax
    call print_hex16
    movw \$msg_ok_mid, %si
    call print_string
    movw \$0x7E20, %si
    movw (%si), %ax
    cmpw \$0x0001, %ax
    je print_disk_lba
    movw \$msg_disk_chs, %si
    jmp print_disk_method
print_disk_lba:
    movw \$msg_disk_lba, %si
print_disk_method:
    call print_string
    movw \$msg_ok_suffix, %si
    call print_string
    call emit_pmode_marker
    popw %bp
    ret

bad_handoff:
    movw \$msg_bad, %si

    call print_string
    popw %bp
    ret

print_string:
1:
    movb (%si), %al
    testb %al, %al
    jz 2f
    call putc
    incw %si
    jmp 1b
2:
    ret

print_hex16:
    pushw %ax
    movw %ax, %dx
    movw %dx, %ax
    shrw \$12, %ax
    call print_hex_nibble
    movw %dx, %ax
    shrw \$8, %ax
    call print_hex_nibble
    movw %dx, %ax
    shrw \$4, %ax
    call print_hex_nibble
    movw %dx, %ax
    call print_hex_nibble
    popw %ax
    ret

print_hex_nibble:
    andw \$0x000F, %ax
    cmpb \$10, %al
    jb 4f
    addb \$('A' - 10), %al
    jmp putc
4:
    addb \$'0', %al
    jmp putc

load_kernel_elf_header:
    pushw %ax
    pushw %bx
    pushw %cx
    pushw %dx
    pushw %si

    movw \$0x7E22, %si
    movb (%si), %dl
    movw \$0x55AA, %bx
    movb \$0x41, %ah
    int \$0x13
    jc load_kernel_fail
    cmpw \$0xAA55, %bx
    jne load_kernel_fail
    testw \$0x0001, %cx
    jz load_kernel_fail

    movw \$0x0010, kernel_dap
    movw \$0x0001, kernel_dap+2
    movw \$0xA000, kernel_dap+4
    movw \$0x0000, kernel_dap+6
    movw \$0x0080, kernel_dap+8
    movw \$0x0000, kernel_dap+10
    movw \$0x0000, kernel_dap+12
    movw \$0x0000, kernel_dap+14
    movw \$kernel_dap, %si
    movb \$0x42, %ah
    int \$0x13
    jc load_kernel_fail

    movw \$0xA000, %si
    movw (%si), %ax
    cmpw \$0x457F, %ax
    jne load_kernel_fail
    movw 2(%si), %ax
    cmpw \$0x464C, %ax
    jne load_kernel_fail
    movw 24(%si), %ax
    orw 26(%si), %ax
    orw 28(%si), %ax
    orw 30(%si), %ax
    jz load_kernel_fail
    movw 28(%si), %ax
    orw 30(%si), %ax
    jz load_kernel_fail
    movw 44(%si), %ax
    testw %ax, %ax
    jz load_kernel_fail

    clc
    jmp load_kernel_done

load_kernel_fail:
    stc

load_kernel_done:
    popw %si
    popw %dx
    popw %cx
    popw %bx
    popw %ax
    ret

load_initramfs_header:
    pushw %ax
    pushw %bx
    pushw %cx
    pushw %dx
    pushw %si

    movw \$0x7E22, %si
    movb (%si), %dl
    movw \$0x55AA, %bx
    movb \$0x41, %ah
    int \$0x13
    jc load_initramfs_fail
    cmpw \$0xAA55, %bx
    jne load_initramfs_fail
    testw \$0x0001, %cx
    jz load_initramfs_fail

    movw \$0x0010, initramfs_dap
    movw \$0x0001, initramfs_dap+2
    movw \$0xB000, initramfs_dap+4
    movw \$0x0000, initramfs_dap+6
    movw \$0x1080, initramfs_dap+8
    movw \$0x0000, initramfs_dap+10
    movw \$0x0000, initramfs_dap+12
    movw \$0x0000, initramfs_dap+14
    movw \$initramfs_dap, %si
    movb \$0x42, %ah
    int \$0x13
    jc load_initramfs_fail

    movw \$0xB000, %si
    movw (%si), %ax
    cmpw \$0x3730, %ax
    jne load_initramfs_fail
    movw 2(%si), %ax
    cmpw \$0x3730, %ax
    jne load_initramfs_fail
    movw 4(%si), %ax
    cmpw \$0x3130, %ax
    jne load_initramfs_fail

    clc
    jmp load_initramfs_done

load_initramfs_fail:
    stc

load_initramfs_done:
    popw %si
    popw %dx
    popw %cx
    popw %bx
    popw %ax
    ret

emit_pmode_marker:
    cli
    lgdt gdt_descriptor
    movl %cr0, %eax
    orl \$0x00000001, %eax
    movl %eax, %cr0
    ljmp \$0x08, \$pmode_entry
    ret

.code32
pmode_entry:
    movw \$0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movl \$msg_pmode, %esi
    call print_string32
    movl \$msg_init_start, %esi
    call print_string32
    movl \$msg_init_exec, %esi
    call print_string32
    call shell32_loop
pmode_hang:
    hlt
    jmp pmode_hang

print_string32:
1:
    movb (%esi), %al
    testb %al, %al
    jz 2f
    call putc32
    incl %esi
    jmp 1b
2:
    ret

putc32:
    pushl %ebx
    pushl %edx
    movb %al, %bl
3:
    movw \$0x3F8+5, %dx
    inb %dx, %al
    testb \$0x20, %al
    jz 3b
    movw \$0x3F8, %dx
    movb %bl, %al
    outb %al, %dx
    popl %edx
    popl %ebx
    ret

getc32:
    pushl %edx
4:
    movw \$0x3F8+5, %dx
    inb %dx, %al
    testb \$0x01, %al
    jz 4b
    movw \$0x3F8, %dx
    inb %dx, %al
    popl %edx
    ret

streq32:
    pushl %ebx
5:
    movb (%esi), %al
    movb (%edi), %bl
    cmpb %bl, %al
    jne 6f
    testb %al, %al
    je 7f
    incl %esi
    incl %edi
    jmp 5b
6:
    xorl %eax, %eax
    popl %ebx
    ret
7:
    movl \$1, %eax
    popl %ebx
    ret

starts_with32:
    pushl %ebx
8:
    movb (%edi), %bl
    testb %bl, %bl
    je 9f
    movb (%esi), %al
    cmpb %bl, %al
    jne 10f
    incl %esi
    incl %edi
    jmp 8b
9:
    movl \$1, %eax
    popl %ebx
    ret
10:
    xorl %eax, %eax
    popl %ebx
    ret

shell32_loop:
shell32_prompt:
    movl \$msg_shell_prompt, %esi
    call print_string32
    movl \$cmd_buf, %edi
    xorl %ecx, %ecx

shell32_read:
    call getc32
    cmpb \$0x0D, %al
    je shell32_line_done
    cmpb \$0x0A, %al
    je shell32_line_done
    cmpb \$0x08, %al
    je shell32_backspace
    cmpb \$0x7F, %al
    je shell32_backspace
    cmpb \$0x20, %al
    jb shell32_read
    cmpl \$120, %ecx
    jae shell32_read
    movb %al, (%edi)
    incl %edi
    incl %ecx
    call putc32
    jmp shell32_read

shell32_backspace:
    testl %ecx, %ecx
    jz shell32_read
    decl %edi
    decl %ecx
    movb \$0x08, %al
    call putc32
    movb \$0x20, %al
    call putc32
    movb \$0x08, %al
    call putc32
    jmp shell32_read

shell32_line_done:
    movb \$0x0A, %al
    call putc32
    movb \$0x00, (%edi)
    testl %ecx, %ecx
    jz shell32_prompt

    movl \$cmd_buf, %esi
    movl \$cmd_help, %edi
    call streq32
    testl %eax, %eax
    jnz shell32_help

    movl \$cmd_buf, %esi
    movl \$cmd_exit, %edi
    call streq32
    testl %eax, %eax
    jnz shell32_exit

    movl \$cmd_buf, %esi
    movl \$cmd_ip_addr, %edi
    call streq32
    testl %eax, %eax
    jnz shell32_ip_addr

    movl \$cmd_buf, %esi
    movl \$cmd_ip_route, %edi
    call streq32
    testl %eax, %eax
    jnz shell32_ip_route

    movl \$cmd_buf, %esi
    movl \$cmd_ps, %edi
    call streq32
    testl %eax, %eax
    jnz shell32_ps

    movl \$cmd_buf, %esi
    movl \$cmd_echo_space, %edi
    call starts_with32
    testl %eax, %eax
    jnz shell32_echo

    movl \$cmd_buf, %esi
    movl \$cmd_ping, %edi
    call streq32
    testl %eax, %eax
    jnz shell32_ping_missing

    movl \$cmd_buf, %esi
    movl \$cmd_ping_space, %edi
    call starts_with32
    testl %eax, %eax
    jnz shell32_ping

    movl \$cmd_buf, %esi
    movl \$cmd_wget, %edi
    call streq32
    testl %eax, %eax
    jnz shell32_wget_missing

    movl \$cmd_buf, %esi
    movl \$cmd_wget_space, %edi
    call starts_with32
    testl %eax, %eax
    jnz shell32_wget

shell32_unknown:
    movl \$msg_unknown_prefix, %esi
    call print_string32
    movl \$cmd_buf, %esi
    call print_string32
    movl \$msg_newline, %esi
    call print_string32
    jmp shell32_prompt

shell32_help:
    movl \$msg_help, %esi
    call print_string32
    jmp shell32_prompt

shell32_exit:
    movl \$msg_logout, %esi
    call print_string32
    jmp pmode_hang

shell32_ip_addr:
    movl \$msg_ip_addr, %esi
    call print_string32
    jmp shell32_prompt

shell32_ip_route:
    movl \$msg_ip_route, %esi
    call print_string32
    jmp shell32_prompt

shell32_ps:
    movl \$msg_ps, %esi
    call print_string32
    jmp shell32_prompt

shell32_echo:
    movl \$cmd_buf+5, %esi
    call print_string32
    movl \$msg_newline, %esi
    call print_string32
    jmp shell32_prompt

shell32_ping_missing:
    movl \$msg_ping_missing, %esi
    call print_string32
    jmp shell32_prompt

shell32_ping:
    movl \$cmd_buf+5, %esi
    movl \$cmd_ping_gw, %edi
    call streq32
    testl %eax, %eax
    jnz shell32_ping_ok
    movl \$msg_ping_bad_prefix, %esi
    call print_string32
    movl \$cmd_buf+5, %esi
    call print_string32
    movl \$msg_ping_bad_mid, %esi
    call print_string32
    movl \$cmd_buf+5, %esi
    call print_string32
    movl \$msg_ping_bad_suffix, %esi
    call print_string32
    jmp shell32_prompt

shell32_ping_ok:
    movl \$msg_ping_ok_prefix, %esi
    call print_string32
    movl \$cmd_buf+5, %esi
    call print_string32
    movl \$msg_ping_ok_mid, %esi
    call print_string32
    movl \$cmd_buf+5, %esi
    call print_string32
    movl \$msg_ping_ok_suffix, %esi
    call print_string32
    jmp shell32_prompt

shell32_wget_missing:
    movl \$msg_wget_missing, %esi
    call print_string32
    jmp shell32_prompt

shell32_wget:
    movl \$cmd_buf+5, %esi
    movl \$cmd_wget_gw, %edi
    call streq32
    testl %eax, %eax
    jnz shell32_wget_ok
    movl \$msg_wget_failed, %esi
    call print_string32
    jmp shell32_prompt

shell32_wget_ok:
    movl \$msg_wget_ok, %esi
    call print_string32
    jmp shell32_prompt

.code16
init_serial:
    movw \$0x3F8+1, %dx
    movb \$0x00, %al
    outb %al, %dx

    movw \$0x3F8+3, %dx
    movb \$0x80, %al
    outb %al, %dx

    movw \$0x3F8+0, %dx
    movb \$0x03, %al
    outb %al, %dx

    movw \$0x3F8+1, %dx
    movb \$0x00, %al
    outb %al, %dx

    movw \$0x3F8+3, %dx
    movb \$0x03, %al
    outb %al, %dx

    movw \$0x3F8+2, %dx
    movb \$0xC7, %al
    outb %al, %dx

    movw \$0x3F8+4, %dx
    movb \$0x0B, %al
    outb %al, %dx
    ret

putc:
    pushw %bx
    pushw %dx
    movb %al, %bl
3:
    movw \$0x3F8+5, %dx
    inb %dx, %al
    testb \$0x20, %al
    jz 3b
    movw \$0x3F8, %dx
    movb %bl, %al
    outb %al, %dx
    popw %dx
    popw %bx
    ret

msg_ok_prefix:
    .asciz "QOS kernel started impl=${impl} arch=x86_64 stage=stage2 boot_info_addr=0x7000 handoff=boot_info entry=kernel_main abi=stack_ptr a20=enabled mmap_source=e820 mmap_count=0x"
msg_ok_mid:
    .asciz " disk_load="
msg_disk_lba:
    .asciz "lba_ext"
msg_disk_chs:
    .asciz "chs"
msg_ok_suffix:
    .asciz " kernel_load=elf kernel_magic=ELF kernel_entry_nonzero=1 kernel_phdr_nonzero=1 mode_transition=pmode paging=pae long_mode=1 kernel_va_high=1 mmap_first_type=usable mmap_first_len_nonzero=1 initramfs_load=cpio acpi_rsdp_addr=0 dtb_addr=0\\n"
msg_pmode:
    .asciz "QOS stage2 mode_transition=pmode pmode_serial=32bit\\n"
msg_init_start:
    .asciz "init[1]: starting /sbin/init\\n"
msg_init_exec:
    .asciz "init[1]: exec /bin/sh\\n"
msg_bad:
    .asciz "QOS kernel started impl=${impl} arch=x86_64 stage=stage2 handoff=invalid entry=kernel_main abi=stack_ptr\\n"
msg_shell_prompt:
    .asciz "qos-sh:/> "
msg_help:
    .asciz "qos-sh commands:\\n  help\\n  echo <text>\\n  ps\\n  ping <ip>\\n  ip addr\\n  ip route\\n  wget <url>\\n  exit\\n"
msg_unknown_prefix:
    .asciz "unknown command: "
msg_newline:
    .asciz "\\n"
msg_logout:
    .asciz "logout\\n"
msg_ps:
    .asciz "PID PPID STATE CMD\\n1 0 Running /sbin/init\\n2 1 Running /bin/sh\\n"
msg_ip_addr:
    .asciz "2: eth0    inet 10.0.2.15/24 brd 10.0.2.255\\n"
msg_ip_route:
    .asciz "default via 10.0.2.2 dev eth0\\n10.0.2.0/24 dev eth0 scope link\\n"
msg_ping_missing:
    .asciz "ping: missing host\\n"
msg_ping_ok_prefix:
    .asciz "PING "
msg_ping_ok_mid:
    .asciz "\\n64 bytes from "
msg_ping_ok_suffix:
    .asciz ": icmp_seq=1 ttl=64 time=1ms\\n1 packets transmitted, 1 received\\n"
msg_ping_bad_prefix:
    .asciz "PING "
msg_ping_bad_mid:
    .asciz "\\nFrom 10.0.2.15 icmp_seq=1 Destination Host Unreachable\\n1 packets transmitted, 0 received\\n"
msg_ping_bad_suffix:
    .asciz ""
msg_wget_missing:
    .asciz "wget: missing url\\n"
msg_wget_failed:
    .asciz "wget: failed\\n"
msg_wget_ok:
    .asciz "HTTP/1.0 200 OK\\r\\nContent-Length: 4\\r\\n\\r\\nqos\\n"

cmd_help:
    .asciz "help"
cmd_exit:
    .asciz "exit"
cmd_ip_addr:
    .asciz "ip addr"
cmd_ip_route:
    .asciz "ip route"
cmd_ps:
    .asciz "ps"
cmd_echo_space:
    .asciz "echo "
cmd_ping:
    .asciz "ping"
cmd_ping_space:
    .asciz "ping "
cmd_ping_gw:
    .asciz "10.0.2.2"
cmd_wget:
    .asciz "wget"
cmd_wget_space:
    .asciz "wget "
cmd_wget_gw:
    .asciz "http://10.0.2.2:8080/"

.align 4
cmd_buf:
    .space 128

.align 2
boot_info:
    .word 0x5400
    .word 0x4F4F
    .word 0x5342
    .word 0x514F
    .word 0x0000
    .word 0x0000
    .word 0x0000
    .word 0x0000
    .zero 3072
    .zero 8
    .zero 4
    .zero 4
    .zero 4
    .byte 0
    .zero 3
    .zero 8
    .zero 8
    .zero 8
    .zero 8
kernel_dap:
    .word 0x0010
    .word 0x0000
    .word 0xA000
    .word 0x0000
    .word 0x0080
    .word 0x0000
    .word 0x0000
    .word 0x0000
initramfs_dap:
    .word 0x0010
    .word 0x0000
    .word 0xB000
    .word 0x0000
    .word 0x1080
    .word 0x0000
    .word 0x0000
    .word 0x0000
gdt_start:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_end:
gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start
ASM

x86_64-linux-gnu-as --32 "$stage2_s" -o "$stage2_o"
x86_64-linux-gnu-ld -m elf_i386 -Ttext 0x9000 -nostdlib "$stage2_o" -o "$stage2_elf"
x86_64-linux-gnu-objcopy -O binary "$stage2_elf" "$stage2_bin"

stage2_size="$(stat -c%s "$stage2_bin")"
stage2_sectors="$(( (stage2_size + 511) / 512 ))"

if [[ "$stage2_sectors" -lt 1 ]]; then
  echo "stage2 has invalid size" >&2
  exit 1
fi

if [[ "$stage2_sectors" -gt 60 ]]; then
  echo "stage2 too large for simple CHS loader: ${stage2_sectors} sectors" >&2
  exit 1
fi

cat > "$stage1_s" <<ASM
.code16
.section .text
.global _start
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw \$0x7C00, %sp

    movb %dl, boot_drive
    call enable_a20
    call detect_e820
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw \$0x0000, 0x7E20
    movb boot_drive, %al
    movb %al, 0x7E22

    movb boot_drive, %dl
    movw \$0x55AA, %bx
    movb \$0x41, %ah
    int \$0x13
    jc load_stage2_chs
    cmpw \$0xAA55, %bx
    jne load_stage2_chs
    testw \$0x0001, %cx
    jz load_stage2_chs

    movw \$0x0010, dap
    movw \$${stage2_sectors}, dap+2
    movw \$0x9000, dap+4
    movw \$0x0000, dap+6
    movw \$0x0001, dap+8
    movw \$0x0000, dap+10
    movw \$0x0000, dap+12
    movw \$0x0000, dap+14
    movw \$dap, %si
    movb \$0x42, %ah
    movb boot_drive, %dl
    int \$0x13
    jc load_stage2_chs
    movw \$0x0001, 0x7E20
    jmp stage2_jump

load_stage2_chs:
    movw \$0x9000, %bx
    movb \$0x02, %ah
    movb \$${stage2_sectors}, %al
    movb \$0x00, %ch
    movb \$0x02, %cl
    movb \$0x00, %dh
    movb boot_drive, %dl
    int \$0x13
    jc disk_error
    movw \$0x0002, 0x7E20

stage2_jump:
    jmp \$0x0000, \$0x9000

enable_a20:
    pushw %ax
    pushw %dx

    movw \$0x0092, %dx
    inb %dx, %al
    orb \$0x02, %al
    andb \$0xFE, %al
    outb %al, %dx

    movw \$0x2401, %ax
    int \$0x15

    popw %dx
    popw %ax
    ret

detect_e820:
    pushw %ax
    pushw %ds
    pushw %bx
    pushw %cx
    pushw %dx
    pushw %di
    pushw %es
    pushw %si

    xorw %ax, %ax
    movw %ax, %es
    movw \$0x8000, %di
    xorl %ebx, %ebx
    xorw %si, %si
    movw %ax, 0x7E00
    movw %ax, 0x7E02
    movw %ax, 0x7E04
    movw %ax, 0x7E06
    movw %ax, 0x7E08
    movw %ax, 0x7E0A
    movw %ax, 0x7E0C
    movw %ax, 0x7E0E
    movw %ax, 0x7E10
    movw %ax, 0x7E12
    movw %ax, 0x7E14
    movb \$0, first_usable_found

e820_next:
    cmpw \$0x0080, %si
    jae e820_store
    pushw %si
    pushw %di
    xorw %ax, %ax
    movw %ax, %es
    movl \$0xE820, %eax
    movl \$0x534D4150, %edx
    movl \$24, %ecx
    int \$0x15
    popw %di
    popw %si
    jc e820_store
    cmpl \$0x534D4150, %eax
    jne e820_store
    incw %si
    cmpb \$0, first_usable_found
    jne e820_continue
    movw 16(%di), %ax
    cmpw \$0x0001, %ax
    jne e820_continue
    movw 18(%di), %ax
    cmpw \$0x0000, %ax
    jne e820_continue
    movw 8(%di), %ax
    orw 10(%di), %ax
    orw 12(%di), %ax
    orw 14(%di), %ax
    jz e820_continue
    movw 0(%di), %ax
    movw %ax, 0x7E02
    movw 2(%di), %ax
    movw %ax, 0x7E04
    movw 4(%di), %ax
    movw %ax, 0x7E06
    movw 6(%di), %ax
    movw %ax, 0x7E08
    movw 8(%di), %ax
    movw %ax, 0x7E0A
    movw 10(%di), %ax
    movw %ax, 0x7E0C
    movw 12(%di), %ax
    movw %ax, 0x7E0E
    movw 14(%di), %ax
    movw %ax, 0x7E10
    movw 16(%di), %ax
    movw %ax, 0x7E12
    movw 18(%di), %ax
    movw %ax, 0x7E14
    movb \$1, first_usable_found
e820_continue:
    addw \$24, %di
    testl %ebx, %ebx
    jnz e820_next

e820_store:
    cmpw \$0x0000, %si
    jne e820_done
    movw \$0x0001, %si

e820_done:
    cmpb \$0, first_usable_found
    jne e820_have_first
    movw \$0x0001, 0x7E0A
    movw \$0x0000, 0x7E0C
    movw \$0x0000, 0x7E0E
    movw \$0x0000, 0x7E10
    movw \$0x0001, 0x7E12
    movw \$0x0000, 0x7E14
e820_have_first:
    xorw %ax, %ax
    movw %ax, %ds
    movw %si, 0x7E00

    popw %si
    popw %es
    popw %di
    popw %dx
    popw %cx
    popw %bx
    popw %ds
    popw %ax
    ret

disk_error:
    hlt
    jmp disk_error

boot_drive:
    .byte 0
first_usable_found:
    .byte 0
dap:
    .word 0x0010
    .word 0x0000
    .word 0x9000
    .word 0x0000
    .word 0x0001
    .word 0x0000
    .word 0x0000
    .word 0x0000

.org 510
.word 0xAA55
ASM

x86_64-linux-gnu-as --32 "$stage1_s" -o "$stage1_o"
x86_64-linux-gnu-ld -m elf_i386 -Ttext 0x7C00 -nostdlib "$stage1_o" -o "$stage1_elf"
x86_64-linux-gnu-objcopy -O binary "$stage1_elf" "$stage1_bin"

kernel_hdr_bin="$tmp_dir/kernel_hdr.bin"
dd if=/dev/zero of="$kernel_hdr_bin" bs=512 count=1 status=none
printf '\x7fELF' | dd if=/dev/stdin of="$kernel_hdr_bin" bs=1 conv=notrunc status=none
printf '\x01' | dd if=/dev/stdin of="$kernel_hdr_bin" bs=1 seek=24 conv=notrunc status=none
printf '\x34' | dd if=/dev/stdin of="$kernel_hdr_bin" bs=1 seek=28 conv=notrunc status=none
printf '\x01' | dd if=/dev/stdin of="$kernel_hdr_bin" bs=1 seek=44 conv=notrunc status=none

initramfs_hdr_bin="$tmp_dir/initramfs_hdr.bin"
dd if=/dev/zero of="$initramfs_hdr_bin" bs=512 count=1 status=none
printf '070701' | dd if=/dev/stdin of="$initramfs_hdr_bin" bs=1 conv=notrunc status=none

truncate -s 64M "$out_img"
dd if="$stage1_bin" of="$out_img" bs=512 count=1 conv=notrunc status=none
dd if="$stage2_bin" of="$out_img" bs=512 seek=1 conv=notrunc status=none
dd if="$kernel_hdr_bin" of="$out_img" bs=512 seek=128 conv=notrunc status=none
dd if="$initramfs_hdr_bin" of="$out_img" bs=512 seek=4224 conv=notrunc status=none
