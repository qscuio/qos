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

unsigned char trampoline_code[] = {
    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, <address of custom_code>
    0xff, 0xe0                                                // jmp rax
};

unsigned char original_code[sizeof(trampoline_code)];

void custom_code() {
    printf("This is custom code before printf.\n");
}

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

void inject_trampoline(pid_t target_pid, void* target_addr, void* hook_addr) {
    struct user_regs_struct regs, saved_regs;
    if (ptrace(PTRACE_GETREGS, target_pid, NULL, &regs) == -1) {
        perror("ptrace getregs");
        exit(1);
    }
    memcpy(&saved_regs, &regs, sizeof(struct user_regs_struct));

    // 保存目标函数的原始代码
    for (size_t i = 0; i < sizeof(trampoline_code); i += sizeof(long)) {
        long data = ptrace(PTRACE_PEEKTEXT, target_pid, target_addr + i, NULL);
        if (data == -1 && errno != 0) {
            perror("ptrace peektext");
            exit(1);
        }
        memcpy(&original_code[i], &data, sizeof(long));
    }

    // 构建 trampoline 代码
    unsigned long hook_addr_long = (unsigned long)hook_addr;
    memcpy(trampoline_code + 2, &hook_addr_long, sizeof(void*));

    // 注入 trampoline 代码
    for (size_t i = 0; i < sizeof(trampoline_code); i += sizeof(long)) {
        long data;
        memcpy(&data, trampoline_code + i, sizeof(long));
        if (ptrace(PTRACE_POKETEXT, target_pid, target_addr + i, (void*)data) == -1) {
            perror("ptrace poketext");
            exit(1);
        }
    }

    // 恢复寄存器并继续运行
    if (ptrace(PTRACE_SETREGS, target_pid, NULL, &saved_regs) == -1) {
        perror("ptrace setregs");
        exit(1);
    }
    if (ptrace(PTRACE_CONT, target_pid, NULL, NULL) == -1) {
        perror("ptrace cont");
        exit (1);
    }
}

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
    void* printf_address = get_remote_function_address(target_pid, "libc.so.6", "printf");

    printf("printf remote address %p\n", printf_address);
    // 注入 trampoline 代码
    inject_trampoline(target_pid, printf_address, (void*)custom_code);

    // 分离目标进程
    if (ptrace(PTRACE_DETACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace detach");
        return 1;
    }

    printf("Injected successfully.\n");
    return 0;
}

