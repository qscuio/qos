#define _GNU_SOURCE
#include <link.h>
#include <stdio.h>

// dl_iterate_phdr 是一个 GNU C 库函数，用于迭代当前进程的所有加载程序头。它常用于实现一些低级别的调试或分析工具，来检查加载的动态库和它们的程序头信息。这个函数在 <link.h> 头文件中声明。

// 定义回调函数
int callback(struct dl_phdr_info *info, size_t size, void *data) {
    printf("Name: %s\n", info->dlpi_name);
    printf("Address: %p\n", (void *)info->dlpi_addr);
    for (int i = 0; i < info->dlpi_phnum; i++) {
        printf("\tHeader %d: type %u, offset %lu, vaddr %p, paddr %p, filesz %lu, memsz %lu, flags %u, align %lu\n",
               i,
               info->dlpi_phdr[i].p_type,
               info->dlpi_phdr[i].p_offset,
               (void *)info->dlpi_phdr[i].p_vaddr,
               (void *)info->dlpi_phdr[i].p_paddr,
               info->dlpi_phdr[i].p_filesz,
               info->dlpi_phdr[i].p_memsz,
               info->dlpi_phdr[i].p_flags,
               info->dlpi_phdr[i].p_align);
    }
    return 0;
}

int main() {
    // 使用 dl_iterate_phdr 迭代所有程序头
    dl_iterate_phdr(callback, NULL);
    return 0;
}

