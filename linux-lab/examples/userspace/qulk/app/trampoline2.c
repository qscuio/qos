#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// 目标函数
void target_function() {
    printf("This is the target function.\n");
}

// 自定义代码
void my_custom_code() {
    printf("This is custom code before the target function.\n");
}

// 保存原始函数的前5字节
unsigned char original_bytes[5];

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
    // 保存原始指令
    memcpy(original_bytes, target_function, 5);

    // 使目标函数地址可写
    make_memory_writable(target_function);

    // 计算跳转偏移
    uintptr_t jump_offset = (uintptr_t)my_custom_code - (uintptr_t)target_function - 5;

    // 写入跳转指令 (0xE9 是 x86 的 near jump 指令)
    unsigned char jmp_instruction[5] = {0xE9, jump_offset & 0xFF, (jump_offset >> 8) & 0xFF, (jump_offset >> 16) & 0xFF, (jump_offset >> 24) & 0xFF};
    memcpy(target_function, jmp_instruction, 5);
}

// 恢复原始函数
void remove_hook() {
    // 使目标函数地址可写
    make_memory_writable(target_function);

    // 恢复原始指令
    memcpy(target_function, original_bytes, 5);
}

int main() {
    // 安装钩子
    install_hook();

    // 调用目标函数
    target_function();  // 输出应为：自定义代码

    // 恢复原始函数
    remove_hook();

    // 再次调用目标函数
    target_function();  // 输出应为：目标函数

    return 0;
}

