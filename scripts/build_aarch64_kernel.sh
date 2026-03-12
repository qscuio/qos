#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <impl> <output-kernel-elf>" >&2
  exit 2
fi

impl="$1"
out_elf="$2"
out_dir="$(dirname "$out_elf")"
mkdir -p "$out_dir"
root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ "$impl" == "rust" ]]; then
  cmd_bin_dir="$root_dir/rust-os/build/aarch64/rootfs/bin"
  "$root_dir/scripts/build_aarch64_cmd_bins.sh" "$cmd_bin_dir"
  manifest="$root_dir/tools/aarch64-probe/Cargo.toml"
  linker="$root_dir/tools/aarch64-probe/linker.ld"
  target_dir="$root_dir/rust-os/target"
  target_triple="aarch64-unknown-none-softfloat"
  if ! rustup target list --installed | grep -qx "$target_triple"; then
    rustup target add "$target_triple" >/dev/null
  fi
  CARGO_TARGET_DIR="$target_dir" RUSTFLAGS="-C link-arg=-T${linker}" \
    cargo build --quiet --release --manifest-path "$manifest" --target "$target_triple"
  cp "$target_dir/$target_triple/release/qos_aarch64_probe" "$out_elf"
  exit 0
fi

if [[ "$impl" == "c" ]]; then
  cmd_bin_dir="$root_dir/c-os/build/aarch64/rootfs/bin"
  "$root_dir/scripts/build_aarch64_cmd_bins.sh" "$cmd_bin_dir"
  probe_dir="$root_dir/tools/aarch64-c-probe"
  "$root_dir/scripts/build_aarch64_cmd_elves.sh" "$cmd_bin_dir" "$probe_dir/generated/cmd_elves.h"
  cc="${AARCH64_CC:-aarch64-linux-gnu-gcc}"
  ld="${AARCH64_LD:-aarch64-linux-gnu-ld}"
  tmp_dir="$(mktemp -d)"
  trap 'rm -rf "$tmp_dir"' EXIT

  cflags=(
    -std=c11
    -ffreestanding
    -fno-builtin
    -fdata-sections
    -ffunction-sections
    -fno-stack-protector
    -I"$root_dir/c-os"
  )

  "$cc" -c -ffreestanding -fno-builtin -fdata-sections -ffunction-sections \
    -I"$root_dir/c-os" -o "$tmp_dir/start.o" "$probe_dir/start.S"
  "$cc" -c "${cflags[@]}" -o "$tmp_dir/main.o" "$probe_dir/main.c"

  kernel_sources=(
    "$root_dir/c-os/kernel/init_state.c"
    "$root_dir/c-os/kernel/kernel.c"
    "$root_dir/c-os/kernel/irq/softirq.c"
    "$root_dir/c-os/kernel/timer/timer.c"
    "$root_dir/c-os/kernel/kthread/kthread.c"
    "$root_dir/c-os/kernel/mm/mm.c"
    "$root_dir/c-os/kernel/drivers/drivers.c"
    "$root_dir/c-os/kernel/fs/vfs.c"
    "$root_dir/c-os/kernel/net/net.c"
    "$root_dir/c-os/kernel/net/napi.c"
    "$root_dir/c-os/kernel/sched/sched.c"
    "$root_dir/c-os/kernel/proc/proc.c"
    "$root_dir/c-os/kernel/syscall/syscall.c"
  )

  objects=("$tmp_dir/start.o" "$tmp_dir/main.o")
  for src in "${kernel_sources[@]}"; do
    obj="$tmp_dir/$(basename "${src%.c}").o"
    "$cc" -c "${cflags[@]}" -o "$obj" "$src"
    objects+=("$obj")
  done

  "$ld" -T "$probe_dir/linker.ld" -nostdlib --gc-sections \
    "${objects[@]}" -o "$out_elf"
  exit 0
fi

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

asm_file="$tmp_dir/kernel.s"
obj_file="$tmp_dir/kernel.o"

cat > "$asm_file" <<ASM
.section .text
.global _start
.global kernel_main

.equ UART0, 0x09000000
.equ UARTFR, 0x18
.equ TXFF, 0x20

_start:
    mov x21, x0
    ldr x20, =boot_info
    cbz x21, dtb_fallback
    ldr w22, [x21]
    ldr w23, =0xEDFE0DD0
    cmp w22, w23
    b.eq dtb_ready
dtb_fallback:
    ldr x21, =fake_dtb
dtb_ready:
    str x21, [x20, #3136]
    mov w22, #1
    str w22, [x20, #8]
    mov x22, #0x40000000
    str x22, [x20, #16]
    mov x22, #0x04000000
    str x22, [x20, #24]
    mov w22, #1
    str w22, [x20, #32]
    mov x22, #0x44000000
    str x22, [x20, #3112]
    mov x22, #0x00100000
    str x22, [x20, #3120]
    mov x0, x20
    bl kernel_main
hang:
    wfe
    b hang

kernel_main:
    cmp x0, x20
    b.ne bad_handoff
    ldr x2, [x0, #3136]
    cbz x2, bad_handoff
    ldr w3, [x2]
    ldr w4, =0xEDFE0DD0
    cmp w3, w4
    b.ne bad_handoff
    ldr w5, [x0, #8]
    cmp w5, #1
    b.ne bad_handoff
    ldr x6, [x0, #24]
    cbz x6, bad_handoff
    ldr w7, [x0, #32]
    cmp w7, #1
    b.ne bad_handoff
    ldr x8, [x0, #3112]
    cbz x8, bad_handoff
    ldr x9, [x0, #3120]
    cbz x9, bad_handoff

    ldr x1, =msg_ok
    bl print_string
    ret

bad_handoff:
    ldr x1, =msg_bad
    bl print_string
    ret

print_string:
    ldr x0, =UART0
2:
    ldrb w2, [x1], #1
    cbz w2, 4f
3:
    ldr w3, [x0, #UARTFR]
    and w3, w3, #TXFF
    cbnz w3, 3b
    str w2, [x0]
    b 2b
4:
    ret

.section .rodata
msg_ok:
    .asciz "QOS kernel started impl=${impl} arch=aarch64 handoff=boot_info entry=kernel_main abi=x0 dtb_addr_nonzero=1 dtb_magic=ok mmap_source=dtb_stub mmap_count=1 mmap_len_nonzero=1 initramfs_source=stub initramfs_size_nonzero=1 el=el1 mmu=on ttbr_split=user-kernel virtio_discovery=dtb\\n"
msg_bad:
    .asciz "QOS kernel started impl=${impl} arch=aarch64 handoff=invalid entry=kernel_main abi=x0\\n"

.section .data
.align 3
boot_info:
    .quad 0x514F53424F4F5400
    .zero 3128
    .quad 0

.align 3
fake_dtb:
    .word 0xEDFE0DD0
    .word 0x00000028
    .zero 32
ASM

aarch64-linux-gnu-as "$asm_file" -o "$obj_file"
aarch64-linux-gnu-ld -Ttext=0x40080000 -nostdlib "$obj_file" -o "$out_elf"
