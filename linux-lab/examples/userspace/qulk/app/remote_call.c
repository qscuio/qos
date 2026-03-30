#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>

// 通过ptrace连接到一个正在运行的进程，调用其中的一个函数并获取其结果，我们需要进行以下几个步骤：
//
// 获取目标进程的进程ID（PID）。
// 使用ptrace附加到目标进程。
// 暂停目标进程，保存其上下文。
// 查找函数foo在目标进程中的地址。
// 修改目标进程的指令指针来调用foo函数。
// 恢复目标进程的上下文并继续执行。
// 获取foo函数的返回值。

// 获取目标进程的模块基址
void* get_module_base(pid_t pid, const char* module_name) {
    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return NULL;
    }

    char line[1024];
    void* base_addr = NULL;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, module_name)) {
            char* base_addr_str = strtok(line, "-");
            base_addr = (void*)strtoul(base_addr_str, NULL, 16);
            break;
        }
    }

    fclose(fp);
    return base_addr;
}

// 获取目标进程中定义的符号地址 (进程本地符号)
void* get_function_address(pid_t pid, const char* module_name, const char* function_name) {
    void* base_addr = get_module_base(pid, module_name);
    if (!base_addr) {
        fprintf(stderr, "Could not find base address of module %s\n", module_name);
        return NULL;
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/root%s", pid, module_name);

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return NULL;
    }

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library initialization failed: %s\n", elf_errmsg(-1));
        close(fd);
        return NULL;
    }

    Elf* elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) {
        fprintf(stderr, "elf_begin() failed: %s\n", elf_errmsg(-1));
        close(fd);
        return NULL;
    }

    Elf_Scn* scn = NULL;
    GElf_Shdr shdr;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            fprintf(stderr, "gelf_getshdr() failed: %s\n", elf_errmsg(-1));
            elf_end(elf);
            close(fd);
            return NULL;
        }
        if (shdr.sh_type == SHT_SYMTAB) {
            break;
        }
    }

    if (!scn) {
        fprintf(stderr, "No symbol table found\n");
        elf_end(elf);
        close(fd);
        return NULL;
    }

    Elf_Data* data = elf_getdata(scn, NULL);
    if (!data) {
        fprintf(stderr, "elf_getdata() failed: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return NULL;
    }

    int count = shdr.sh_size / shdr.sh_entsize;
    for (int i = 0; i < count; i++) {
        GElf_Sym sym;
        if (gelf_getsym(data, i, &sym) != &sym) {
            fprintf(stderr, "gelf_getsym() failed: %s\n", elf_errmsg(-1));
            continue;
        }

        if (strcmp(elf_strptr(elf, shdr.sh_link, sym.st_name), function_name) == 0) {
            elf_end(elf);
            close(fd);
            return (void*)((unsigned long)base_addr + sym.st_value);
        }
    }

    elf_end(elf);
    close(fd);
    return NULL;
}

// 获取目标进程的符号地址（第三方库)
void* get_remote_addr(pid_t pid, const char* module_name, const char* sym_name) {
    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return NULL;
    }

    char line[1024];
    void* addr = NULL;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, module_name)) {
            char* base_addr_str = strtok(line, "-");
            void* base_addr = (void*)strtoul(base_addr_str, NULL, 16);
            
            void* handle = dlopen(module_name, RTLD_LAZY);
            if (!handle) {
                fclose(fp);
                return NULL;
            }
            void* local_sym_addr = dlsym(handle, sym_name);
            if (!local_sym_addr) {
                dlclose(handle);
                fclose(fp);
                return NULL;
            }

            addr = (void*)((unsigned long)base_addr + (unsigned long)local_sym_addr);
            dlclose(handle);
            break;
        }
    }

    fclose(fp);
    return addr;
}

// 在目标进程中写入数据
void write_remote_memory(pid_t pid, void* addr, void* data, size_t size) {
    size_t bytes_written = 0;
    while (bytes_written < size) {
        if (ptrace(PTRACE_POKETEXT, pid, addr + bytes_written, *(long*)(data + bytes_written)) == -1) {
            perror("ptrace poketext");
            exit(1);
        }
        bytes_written += sizeof(long);
    }
}

// 从目标进程中读取数据
void read_remote_memory(pid_t pid, void* addr, void* data, size_t size) {
    size_t bytes_read = 0;
    while (bytes_read < size) {
        long word = ptrace(PTRACE_PEEKDATA, pid, addr + bytes_read, NULL);
        if (word == -1 && errno) {
            perror("ptrace peekdata");
            exit(1);
        }
        memcpy(data + bytes_read, &word, sizeof(long));
        bytes_read += sizeof(long);
    }
}

// 在目标进程中调用 malloc
void* call_malloc(pid_t pid, size_t size) {
    void* malloc_addr = get_function_address(pid, "/lib/x86_64-linux-gnu/libc.so.6", "malloc");
    if (!malloc_addr) {
        fprintf(stderr, "Could not find address of malloc\n");
        exit(1);
    }

    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace getregs");
        exit(1);
    }

    // Set up arguments for malloc
    regs.rdi = size; // First argument: size
    regs.rip = (unsigned long)malloc_addr;
    
    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace setregs");
        exit(1);
    }

    // Continue the process to execute malloc
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {
        perror("ptrace cont");
        exit(1);
    }
    waitpid(pid, NULL, 0);

    // Read the return value (address of allocated memory)
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace getregs");
        exit(1);
    }
    return (void*)regs.rax;
}

// 在目标进程中调用 free
void call_free(pid_t pid, void* ptr) {
    void* free_addr = get_function_address(pid, "/lib/x86_64-linux-gnu/libc.so.6", "free");
    if (!free_addr) {
        fprintf(stderr, "Could not find address of free\n");
        exit(1);
    }

    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace getregs");
        exit(1);
    }

    // Set up arguments for free
    regs.rdi = (unsigned long)ptr; // First argument: pointer to free
    regs.rip = (unsigned long)free_addr;
    
    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace setregs");
        exit(1);
    }

    // Continue the process to execute free
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {
        perror("ptrace cont");
        exit(1);
    }
    waitpid(pid, NULL, 0);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <pid> <module_name> <function_name> <param1> <param2>\n", argv[0]);
        return 1;
    }

    pid_t target_pid = atoi(argv[1]);
    const char* module_name = argv[2];
    const char* function_name = argv[3];
    int param1 = atoi(argv[4]);
    int param2 = atoi(argv[5]);

    // 1. Attach to the target process
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace attach");
        return 1;
    }
    waitpid(target_pid, NULL, 0);

    // 2. Save the current state of the target process
    struct user_regs_struct regs, original_regs;
    if (ptrace(PTRACE_GETREGS, target_pid, NULL, &regs) == -1) {
        perror("ptrace getregs");
        return 1;
    }
    memcpy(&original_regs, &regs, sizeof(struct user_regs_struct));

    // 3. Get the address of the function 'foo'
    void* remote_func_addr = get_function_address(target_pid, module_name, function_name);
    if (!remote_func_addr) {
        fprintf(stderr, "Could not find address of function %s\n", function_name);
        return 1;
    }
    printf("Function %s address: %p\n", function_name, remote_func_addr);

    // 4. Allocate memory in the target process using malloc
    void* remote_param_addr = call_malloc(target_pid, 2 * sizeof(int));

    // Write parameters to remote memory
    int params[2] = {param1, param2};
    write_remote_memory(target_pid, remote_param_addr, params, sizeof(params));

    // 5. Set up the function call
    regs.rip = (unsigned long)remote_func_addr;
    regs.rdi = (unsigned long)remote_param_addr; // First parameter
    regs.rsi = (unsigned long)remote_param_addr + sizeof(int); // Second parameter
    if (ptrace(PTRACE_SETREGS, target_pid, NULL, &regs) == -1) {
        perror("ptrace setregs");
        return 1;
    }

    // 6. Continue the process to execute the function
    if (ptrace(PTRACE_CONT, target_pid, NULL, NULL) == -1) {
        perror("ptrace cont");
        return 1;
    }

    // 7. Wait for the function to complete
    waitpid(target_pid, NULL, 0);

    // 8. Get the return value from the function call
    if (ptrace(PTRACE_GETREGS, target_pid, NULL, &regs) == -1) {
        perror("ptrace getregs");
        return 1;
    }
    int result = (int)regs.rax;
    printf("Function %s returned: %d\n", function_name, result);

    // 9. Free the allocated memory in the target process
    call_free(target_pid, remote_param_addr);

    // 10. Restore the original state of the target process
    if (ptrace(PTRACE_SETREGS, target_pid, NULL, &original_regs) == -1) {
        perror("ptrace setregs");
        return 1;
    }

    // 11. Detach from the target process
    if (ptrace(PTRACE_DETACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace detach");
        return 1;
    }

    return 0;
}

