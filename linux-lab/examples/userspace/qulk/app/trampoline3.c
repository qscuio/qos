#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <stdarg.h>

#if 0
#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif
#endif

#define JMP_SIZE 5

// 保存原始 printf 函数指针
static int (*original_printf)(const char *format, ...) = NULL;

// 自定义代码
int my_printf(const char *format, ...) {
    printf("This is custom code before printf.\n");

    // 调用原始 printf 函数
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);

    return result;
}

// 使内存页面可写
void make_memory_writable(void *addr) {
    size_t pagesize = getpagesize();
    void *page = (void *)((uintptr_t)addr & ~(pagesize - 1));
    if (mprotect(page, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
        perror("mprotect");
        _exit(1);
    }
}

// 安装钩子
void install_hook() {
    // 获取原始 printf 的地址
    original_printf = dlsym(RTLD_NEXT, "printf");
    if (original_printf == NULL) {
        perror("dlsym");
        return;
    }

    // 计算跳转偏移
    uintptr_t jump_offset = (uintptr_t)my_printf - (uintptr_t)original_printf - JMP_SIZE;

    // 写入跳转指令 (0xE9 是 x86 的 near jump 指令)
    unsigned char jmp_instruction[JMP_SIZE] = {
        0xE9,
        jump_offset & 0xFF,
        (jump_offset >> 8) & 0xFF,
        (jump_offset >> 16) & 0xFF,
        (jump_offset >> 24) & 0xFF
    };

    // 使目标函数地址可写
    make_memory_writable((void *)original_printf);

    // 保存原始指令以便之后恢复 (这里只保存前5字节)
    unsigned char original_bytes[JMP_SIZE];
    memcpy(original_bytes, original_printf, JMP_SIZE);

    // 安装钩子
    memcpy(original_printf, jmp_instruction, JMP_SIZE);
}

// 恢复原始函数
void remove_hook() {
    if (original_printf == NULL) return;

    // 恢复原始指令
    unsigned char original_bytes[JMP_SIZE] = {
        0x55,       // push   %rbp
        0x48, 0x89, // mov    %rsp,%rbp
        0xe5,       // 
        0x48        // 
    };

    make_memory_writable((void *)original_printf);
    memcpy(original_printf, original_bytes, JMP_SIZE);
}

int main() {
    // 安装钩子
    install_hook();

    // 调用 printf
    printf("Hello, World!\n");  // 输出应为：自定义代码 -> Hello, World!

    // 恢复原始函数
    remove_hook();

    // 再次调用 printf
    printf("Hello again!\n");  // 输出应为：Hello again!

    return 0;
}

