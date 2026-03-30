// injector.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>

// 获取目标进程中库函数地址
void* get_remote_function_address(pid_t target_pid, const char* lib_name, const char* func_name) {
    FILE* maps_file;
    char maps_path[256], line[256], lib_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", target_pid);
    maps_file = fopen(maps_path, "r");
    if (!maps_file) {
        perror("fopen");
        exit(1);
    }

    void* local_handle = dlopen(lib_name, RTLD_LAZY);
    void* local_func = dlsym(local_handle, func_name);
    Dl_info dlinfo;
    dladdr(local_func, &dlinfo);

    void* local_base = dlinfo.dli_fbase;
    void* remote_base = NULL;

    while (fgets(line, sizeof(line), maps_file)) {
        if (strstr(line, lib_name)) {
            unsigned long start_addr;
            unsigned long end_addr;
            sscanf(line, "%lx-%lx %*s %*s %*s %*s", &start_addr, &end_addr);
            sscanf(line, "%*s %*s %*s %*s %*s %s", lib_path);
            if (strstr(lib_path, lib_name)) {
                remote_base = (void*)start_addr;
                break;
            }
        }
    }

    fclose(maps_file);
    dlclose(local_handle);

    if (!remote_base) {
        fprintf(stderr, "Remote library base address not found.\n");
        exit(1);
    }

    return (void*)((unsigned long)remote_base + ((unsigned long)local_func - (unsigned long)local_base));
}

// 自定义代码
void custom_code() {
    printf("This is custom code before printf.\n");
}

// 注入器代码
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    pid_t target_pid = atoi(argv[1]);

    // 附加到目标进程
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace attach");
        return 1;
    }

    // 等待目标进程停止
    waitpid(target_pid, NULL, 0);

    // 获取目标进程中 printf 的地址
    void* printf_address = get_remote_function_address(target_pid, "libc.so.6", "puts");

    // 保存原始指令
    unsigned long original_code = ptrace(PTRACE_PEEKTEXT, target_pid, printf_address, NULL);
    if (errno != 0) {
        perror("ptrace peek text");
        return 1;
    }

    // 计算自定义代码跳转地址
    void* custom_code_address = (void*)custom_code;
    long jump_offset = (long)custom_code_address - (long)printf_address - 5;
    unsigned long jump_code = 0xE9 | ((unsigned long)jump_offset << 8);

    // 注入跳转指令
    if (ptrace(PTRACE_POKETEXT, target_pid, printf_address, (void*)jump_code) == -1) {
        perror("ptrace poke text");
        return 1;
    }
#if 0

    // 恢复原始指令
    if (ptrace(PTRACE_POKETEXT, target_pid, printf_address, original_code) == -1) {
        perror("ptrace poke text (restore)");
        return 1;
    }
#endif

    // 分离目标进程
    if (ptrace(PTRACE_DETACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace detach");
        return 1;
    }

    printf("Injected successfully.\n");
    return 0;
}

