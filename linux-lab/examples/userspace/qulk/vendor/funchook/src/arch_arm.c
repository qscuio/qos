/* -*- indent-tabs-mode: nil -*-
 *
 * This file is part of Funchook.
 * https://github.com/kubo/funchook
 *
 * Funchook is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * As a special exception, the copyright holders of this library give you
 * permission to link this library with independent modules to produce an
 * executable, regardless of the license terms of these independent
 * modules, and to copy and distribute the resulting executable under
 * terms of your choice, provided that you also meet, for each linked
 * independent module, the terms and conditions of the license of that
 * module. An independent module is a module which is not derived from or
 * based on this library. If you modify this library, you may extend this
 * exception to your version of the library, but you are not obliged to
 * do so. If you do not wish to do so, delete this exception statement
 * from your version.
 *
 * Funchook is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Funchook. If not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "funchook_internal.h"
#include "disasm.h"

#undef PAGE_SIZE
#define PAGE_SIZE 4096

// imm24 at bit 23~0
#define IMM24_MASK 0x00FFFFFF
#define IMM24_OFFSET(ins) ((int32_t)((ins) << 8) >> 6)

// imm12 at bit 11~0
#define IMM12_MASK 0x00000FFF
#define IMM12_OFFSET(ins) ((int32_t)((ins) << 20) >> 20)
#define IMM12_RESET(ins) ((ins) & ~IMM12_MASK)
#define TO_IMM12(imm12) ((imm12) & IMM12_MASK)

// Rd and Rt at bit 15~12
#define RD_MASK 0x0000F000
#define RD_REGNO(ins) (((ins) & RD_MASK) >> 12)
#define RT_REGNO(ins) (((ins) & RD_MASK) >> 12)

// Rn at bit 19~16
#define RN_MASK 0x000F0000
#define RN_REGNO(ins) (((ins) & RN_MASK) >> 16)
#define TO_RN(regno) ((regno) << 16)

#define RESET_AT(ins, pos) ((ins) & ~(1u << (pos)))
#define INVERT_AT(ins, pos) (((ins) & (1u << (pos))) ^ (1u << (pos)))

typedef struct {
    funchook_t *funchook;
    ip_displacement_t *rip_disp;
    const insn_t *src;
    const insn_t *dst_base;
    insn_t *dst;
} make_trampoline_context_t;

static int to_regno(funchook_t *funchook, uint32_t avail_regs, uint32_t *regno)
{
    if (avail_regs & FUNCHOOK_ARM_REG_R4) {
        *regno = 4;
    } else if (avail_regs & FUNCHOOK_ARM_REG_R5) {
        *regno = 5;
    } else if (avail_regs & FUNCHOOK_ARM_REG_R6) {
        *regno = 6;
    } else if (avail_regs & FUNCHOOK_ARM_REG_R7) {
        *regno = 7;
    } else if (avail_regs & FUNCHOOK_ARM_REG_R8) {
        *regno = 8;
    } else if (avail_regs & FUNCHOOK_ARM_REG_R9) {
        *regno = 9;
    } else if (avail_regs & FUNCHOOK_ARM_REG_R10) {
        *regno = 10;
    } else {
        funchook_set_error_message(funchook, "All caller-saved registers are used.");
        return FUNCHOOK_ERROR_NO_AVAILABLE_REGISTERS;
    }
    return 0;
}

static int funchook_write_relative_4g_jump(funchook_t *funchook, const uint32_t *src, const uint32_t *dst, uint32_t *out)
{
    intptr_t imm = (intptr_t)dst - (intptr_t)src;
    uint32_t imm24 = (uint32_t)imm & IMM24_MASK;

    /* ldr pc, [pc, #-4] */
    out[0] = 0xe51ff004;
    /* addr */
    out[1] = (uint32_t)dst;
    funchook_log(funchook, "  Write relative +/-4G jump 0x%08x -> 0x%08x\n",
                 (size_t)src, (size_t)dst);
    return 0;
}

static int funchook_write_absolute_jump(funchook_t *funchook, uint32_t *src, const uint32_t *dst, uint32_t avail_regs)
{
    uint32_t regno;
    int rv = to_regno(funchook, avail_regs, &regno);
    if (rv != 0) {
        return rv;
    }
    /* ldr rX, [pc, #-4] */
    src[0] = 0xe51f0000 | (regno << 12);
    /* mov pc, rX */
    src[1] = 0xe1a0f000 | regno;
    /* addr */
    *(const uint32_t**)(src + 2) = dst;
    funchook_log(funchook, "  Write absolute jump 0x%08x -> 0x%08x\n",
                 (size_t)src, (size_t)dst);
    return 0;
}

static int funchook_write_jump_with_prehook(funchook_t *funchook, funchook_entry_t *entry, const uint8_t *dst)
{
    static const uint32_t template[TRANSIT_CODE_SIZE] = TRANSIT_CODE_TEMPLATE;
    memcpy(entry->transit, template, sizeof(template));
    extern void funchook_hook_caller_asm(void);
    *(void**)(entry->transit + TRANSIT_HOOK_CALLER_ADDR) = (void*)funchook_hook_caller_asm;
    funchook_log(funchook, "  Write jump 0x%08x -> 0x%08x with hook caller 0x%08x\n",
                 (size_t)entry->transit, (size_t)dst, (size_t)funchook_hook_caller);
    return 0;
}

static size_t target_addr(size_t addr, uint32_t ins, uint8_t insn_id)
{
    switch (insn_id) {
    case FUNCHOOK_ARM_INSN_B:
    case FUNCHOOK_ARM_INSN_BL:
        return addr + IMM24_OFFSET(ins);
    case FUNCHOOK_ARM_INSN_LDR:
    case FUNCHOOK_ARM_INSN_LDRB:
        return addr + IMM12_OFFSET(ins);
    case FUNCHOOK_ARM_INSN_BX:
    case FUNCHOOK_ARM_INSN_BLX:
        return addr + IMM24_OFFSET(ins);
    }
    return 0;
}

int funchook_make_trampoline(funchook_t *funchook, ip_displacement_t *disp, const insn_t *func, insn_t *trampoline, size_t *trampoline_size)
{
    make_trampoline_context_t ctx;
    funchook_disasm_t disasm;
    int rv;
    const funchook_insn_t *insn;
    uint32_t avail_regs = FUNCHOOK_ARM_CORRUPTIBLE_REGS;
    size_t *literal_pool = (size_t*)(trampoline + LITERAL_POOL_OFFSET);

#define LDR_ADDR(regno, addr) do { \
    int imm12__ = (int)((size_t)literal_pool - (size_t)ctx.dst); \
    *(literal_pool++) = (addr); \
    *(ctx.dst++) = 0xe51f0000 | TO_IMM12(imm12__) | (regno << 12); \
} while (0)
#define BR_BY_REG(regno) do { \
    *(ctx.dst++) = 0xe1a0f000 | (regno); \
} while (0)

    memset(disp, 0, sizeof(*disp));
    memset(trampoline, 0, TRAMPOLINE_BYTE_SIZE);
    *trampoline_size = 0;
    ctx.funchook = funchook;
    ctx.src = func;
    ctx.dst_base = ctx.dst = trampoline;

    rv = funchook_disasm_init(&disasm, funchook, func, MAX_INSN_CHECK_SIZE, (size_t)func);
    if (rv != 0) {
        return rv;
    }

    while ((insn = funchook_disasm_next(&disasm)) != NULL) {
        uint32_t ins = *ctx.src;
        uint8_t rd, rn;
        size_t addr;
        uint32_t regno;

        if ((size_t)(ctx.dst - ctx.dst_base) + 8 > TRAMPOLINE_BYTE_SIZE) {
            funchook_set_error_message(funchook, "No space in the trampoline area.");
            return FUNCHOOK_ERROR_TRAMPOLINE_TOO_SMALL;
        }

        switch (insn->id) {
        case FUNCHOOK_ARM_INSN_B:
        case FUNCHOOK_ARM_INSN_BL:
            addr = target_addr((size_t)ctx.src, ins, insn->id);
            if (IN_RANGE_P(addr, func, ctx.src)) {
                disp->start = (size_t)ctx.dst;
                disp->src_start = disp->src_end = ctx.src;
                disp->dst_start = disp->dst_end = ctx.dst;
                disp->range_start = func;
                disp->range_end = ctx.src;
                disp->table_size = 0;
                disp->type = FUNCHOOK_DISP_JMP;
                ctx.dst += 2;
                disp->jmp[disp->jmp_size++] = (size_t)addr;
            } else {
                funchook_write_relative_4g_jump(funchook, (const uint32_t*)ctx.src, (const uint32_t*)addr, ctx.dst);
                ctx.dst += 2;
            }
            break;
        case FUNCHOOK_ARM_INSN_LDR:
        case FUNCHOOK_ARM_INSN_LDRB:
            addr = target_addr((size_t)ctx.src, ins, insn->id);
            if (IN_RANGE_P(addr, func, ctx.src)) {
                disp->start = (size_t)ctx.dst;
                disp->src_start = disp->src_end = ctx.src;
                disp->dst_start = disp->dst_end = ctx.dst;
                disp->range_start = func;
                disp->range_end = ctx.src;
                disp->table_size = 0;
                disp->type = FUNCHOOK_DISP_LDR;
                ctx.dst += 2;
                disp->jmp[disp->jmp_size++] = (size_t)addr;
            } else {
                funchook_write_relative_4g_jump(funchook, (const uint32_t*)ctx.src, (const uint32_t*)addr, ctx.dst);
                ctx.dst += 2;
            }
            break;
        case FUNCHOOK_ARM_INSN_BX:
        case FUNCHOOK_ARM_INSN_BLX:
            addr = target_addr((size_t)ctx.src, ins, insn->id);
            if (IN_RANGE_P(addr, func, ctx.src)) {
                disp->start = (size_t)ctx.dst;
                disp->src_start = disp->src_end = ctx.src;
                disp->dst_start = disp->dst_end = ctx.dst;
                disp->range_start = func;
                disp->range_end = ctx.src;
                disp->table_size = 0;
                disp->type = FUNCHOOK_DISP_JMP;
                ctx.dst += 2;
                disp->jmp[disp->jmp_size++] = (size_t)addr;
            } else {
                funchook_write_relative_4g_jump(funchook, (const uint32_t*)ctx.src, (const uint32_t*)addr, ctx.dst);
                ctx.dst += 2;
            }
            break;
        case FUNCHOOK_ARM_INSN_MOV:
            rd = RD_REGNO(ins);
            rn = RN_REGNO(ins);
            if (avail_regs & FUNCHOOK_ARM_CORRUPTIBLE_REGS) {
                avail_regs &= ~TO_RN(rd);
                *(ctx.dst++) = ins;
            } else {
                funchook_set_error_message(funchook, "MOV instruction not supported.");
                return FUNCHOOK_ERROR_UNSUPPORTED_INSTRUCTION;
            }
            break;
        default:
            funchook_set_error_message(funchook, "Unsupported instruction 0x%08x.", ins);
            return FUNCHOOK_ERROR_UNSUPPORTED_INSTRUCTION;
        }
        ctx.src++;
    }

    if (funchook_write_absolute_jump(funchook, ctx.dst, (const uint32_t*)disasm.target_addr, avail_regs) != 0) {
        return FUNCHOOK_ERROR_UNSUPPORTED_INSTRUCTION;
    }
    ctx.dst += 3;
    *trampoline_size = (size_t)(ctx.dst - ctx.dst_base) * sizeof(*ctx.dst);
    return 0;
}

int funchook_fix_code(funchook_t *funchook, const void *addr, size_t len)
{
    size_t page_start = (size_t)addr & ~(PAGE_SIZE - 1);
    size_t page_end = ((size_t)addr + len + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (mprotect((void*)page_start, page_end - page_start, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        funchook_set_error_message(funchook, "mprotect failed. %s", strerror(errno));
        return FUNCHOOK_ERROR_MPROTECT;
    }

    if (cacheflush((intptr_t)page_start, (intptr_t)page_end, 0) != 0) {
        funchook_set_error_message(funchook, "cacheflush failed. %s", strerror(errno));
        return FUNCHOOK_ERROR_CACHEFLUSH;
    }
    return 0;
}

int funchook_arg_get_int_reg_addr(const funchook_arg_t *arg, size_t index, void **addr)
{
    if (index < 4) {
        *addr = (void*)&arg->u.regs[index];
        return 0;
    }
    return FUNCHOOK_ERROR_OUT_OF_BOUNDS;
}

int funchook_arg_get_flt_reg_addr(const funchook_arg_t *arg, size_t index, void **addr)
{
    if (index < 16) {
        *addr = (void*)&arg->u.regs[4 + index];
        return 0;
    }
    return FUNCHOOK_ERROR_OUT_OF_BOUNDS;
}

int funchook_arg_get_stack_addr(const funchook_arg_t *arg, size_t index, void **addr)
{
    if (index < arg->u.stack.size / sizeof(void*)) {
        *addr = (void*)((char*)arg->u.stack.base + index * sizeof(void*));
        return 0;
    }
    return FUNCHOOK_ERROR_OUT_OF_BOUNDS;
}

