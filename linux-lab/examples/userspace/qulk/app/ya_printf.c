#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>  // For putchar
#include <libunwind.h>
#include <capstone/capstone.h>

// Utility function to print a character
void ya_putchar(char c) {
    putchar(c);
}

// Utility function to print a string
void ya_puts(const char *str) {
    while (*str) {
        ya_putchar(*str++);
    }
}

// Utility function to convert an integer to a string (base 10)
void ya_itoa(int value, char *str) {
    char buffer[20];
    int i = 0, j = 0;
    int is_negative = value < 0;
    
    if (is_negative) {
        value = -value;
    }

    do {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    } while (value > 0);

    if (is_negative) {
        buffer[i++] = '-';
    }

    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
}

// Utility function to convert an unsigned integer to a string (base 16)
void ya_itoa_hex(uintptr_t value, char *str) {
    char buffer[20];
    int i = 0, j = 0;

    do {
        int digit = value % 16;
        buffer[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
        value /= 16;
    } while (value > 0);

    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
}

// Utility function to print an IPv4 address
void print_ipv4(const uint8_t *addr) {
    char buffer[4];
    for (int i = 0; i < 4; ++i) {
        ya_itoa(addr[i], buffer);
        ya_puts(buffer);
        if (i < 3) {
            ya_putchar('.');
        }
    }
}

// Utility function to print an IPv6 address
void print_ipv6(const uint8_t *addr) {
    for (int i = 0; i < 16; i += 2) {
        char buffer[5];
        uintptr_t segment = (addr[i] << 8) | addr[i + 1];
        ya_itoa_hex(segment, buffer);
        ya_puts(buffer);
        if (i < 14) {
            ya_putchar(':');
        }
    }
}

// Utility function to print a MAC address
void print_mac(const uint8_t *addr) {
    for (int i = 0; i < 6; ++i) {
        char buffer[3];
        ya_itoa_hex(addr[i], buffer);
        ya_puts(buffer);
        if (i < 5) {
            ya_putchar(':');
        }
    }
}

// Utility function to print a memory dump
void print_memdump(const void *addr, size_t size) {
    const uint8_t *byte_ptr = (const uint8_t *)addr;
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            char buffer[20];
            ya_itoa_hex((uintptr_t)(byte_ptr + i), buffer);
            ya_puts(buffer);
            ya_puts(": ");
        }

        char buffer[3];
        ya_itoa_hex(byte_ptr[i], buffer);
        ya_puts(buffer);
        ya_putchar(' ');

        if ((i + 1) % 16 == 0) {
            ya_putchar('\n');
        }
    }
    ya_putchar('\n');
}

// Utility function to print a register dump
void print_registers() {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip;

    // Inline assembly to capture the register values
    __asm__ volatile (
        "mov %%rax, %0\n\t"
        "mov %%rbx, %1\n\t"
        "mov %%rcx, %2\n\t"
        "mov %%rdx, %3\n\t"
        "mov %%rsi, %4\n\t"
        "mov %%rdi, %5\n\t"
        "mov %%rbp, %6\n\t"
        "mov %%rsp, %7\n\t"
        //"mov %%r8, %8\n\t"
        //"mov %%r9, %9\n\t"
        //"mov %%r10, %10\n\t"
        //"mov %%r11, %11\n\t"
        //"mov %%r12, %12\n\t"
        //"mov %%r13, %13\n\t"
        //"mov %%r14, %14\n\t"
        //"mov %%r15, %15\n\t"
        //"lea (%%rip), %16\n\t"
        : "=r"(rax), "=r"(rbx), "=r"(rcx), "=r"(rdx), "=r"(rsi), "=r"(rdi),
          "=r"(rbp), "=r"(rsp)
	  // "=r"(r8), "=r"(r9), "=r"(r10), "=r"(r11),
          //"=r"(r12), "=r"(r13), "=r"(r14), "=r"(r15), 
	  //"=r"(rip)
    );

    char buffer[20];
    #define PRINT_REG(name, value) \
        ya_puts(name); ya_puts(": "); ya_itoa_hex(value, buffer); ya_puts(buffer); ya_putchar('\n')

    PRINT_REG("RAX", rax);
    PRINT_REG("RBX", rbx);
    PRINT_REG("RCX", rcx);
    PRINT_REG("RDX", rdx);
    PRINT_REG("RSI", rsi);
    PRINT_REG("RDI", rdi);
    PRINT_REG("RBP", rbp);
    PRINT_REG("RSP", rsp);
    //PRINT_REG("R8", r8);
    //PRINT_REG("R9", r9);
    //PRINT_REG("R10", r10);
    //PRINT_REG("R11", r11);
    //PRINT_REG("R12", r12);
    //PRINT_REG("R13", r13);
    //PRINT_REG("R14", r14);
    //PRINT_REG("R15", r15);
    //PRINT_REG("RIP", rip);

    #undef PRINT_REG
}

// Utility function to print an instruction dump
void print_instructions(void *rip) {
    csh handle;
    cs_insn *insn;
    size_t count;
    uint8_t *code = (uint8_t *)rip;
    size_t code_size = 64;  // Number of bytes to disassemble

    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
        ya_puts("Failed to initialize capstone\n");
        return;
    }

    count = cs_disasm(handle, code, code_size, (uint64_t)rip, 0, &insn);
    if (count > 0) {
        for (size_t i = 0; i < count; i++) {
            char buffer[20];
            ya_itoa_hex(insn[i].address, buffer);
            ya_puts(buffer);
            ya_puts(": ");

            for (size_t j = 0; j < insn[i].size; j++) {
                char byte_buffer[3];
                ya_itoa_hex(insn[i].bytes[j], byte_buffer);
                ya_puts(byte_buffer);
                ya_putchar(' ');
            }

            ya_puts("\t");
            ya_puts(insn[i].mnemonic);
            ya_puts(" ");
            ya_puts(insn[i].op_str);
            ya_putchar('\n');
        }

        cs_free(insn, count);
    } else {
        ya_puts("Failed to disassemble given code!\n");
    }

    cs_close(&handle);
}

void ya_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    const char *p = format;
    while (*p) {
        if (*p == '%' && *(p + 1) != '\0') {
            p++;
            switch (*p) {
                case 'd': {
                    int value = va_arg(args, int);
                    char buffer[20];
                    ya_itoa(value, buffer);
                    ya_puts(buffer);
                    break;
                }
                case 'c': {
                    char value = (char)va_arg(args, int);
                    ya_putchar(value);
                    break;
                }
                case 'h': {
                    short value = (short)va_arg(args, int);
                    char buffer[20];
                    ya_itoa(value, buffer);
                    ya_puts(buffer);
                    break;
                }
                case 'l': {
                    long value = va_arg(args, long);
                    char buffer[20];
                    ya_itoa(value, buffer);
                    ya_puts(buffer);
                    break;
                }
                case 's': {
                    const char *value = va_arg(args, const char *);
                    ya_puts(value);
                    break;
                }
                case 'p': {
                    uintptr_t value = (uintptr_t)va_arg(args, void *);
                    char buffer[20];
                    ya_itoa_hex(value, buffer);
                    ya_puts("0x");
                    ya_puts(buffer);
                    break;
                }
                case '4': {  // IPv4 address
                    const uint8_t *addr = va_arg(args, const uint8_t *);
                    print_ipv4(addr);
                    break;
                }
                case '6': {  // IPv6 address
                    const uint8_t *addr = va_arg(args, const uint8_t *);
                    print_ipv6(addr);
                    break;
                }
                case 'm': {  // MAC address
                    const uint8_t *addr = va_arg(args, const uint8_t *);
                    print_mac(addr);
                    break;
                }
                case 'M': {  // Memory dump
                    const void *addr = va_arg(args, const void *);
                    size_t size = va_arg(args, size_t);
                    print_memdump(addr, size);
                    break;
                }
                case 'R': {  // Register dump
                    print_registers();
                    break;
                }
                case 'I': {  // Instruction dump
                    void *rip = __builtin_return_address(0);
                    print_instructions(rip);
                    break;
                }
                default:
                    ya_putchar('%');
                    ya_putchar(*p);
                    break;
            }
        } else {
            ya_putchar(*p);
        }
        p++;
    }

    va_end(args);
}

int main() {
    int i = 123;
    char c = 'A';
    short s = 456;
    long l = 789L;
    const char *str = "Hello, world!";
    void *ptr = &i;

    uint8_t ipv4[4] = {192, 168, 1, 1};
    uint8_t ipv6[16] = {0x20, 0x01, 0x0d, 0xb8, 0x85, 0xa3, 0x00, 0x00,
                        0x00, 0x00, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x34};
    uint8_t mac[6] = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E};

    ya_printf("Int: %d\n", i);
    ya_printf("Char: %c\n", c);
    ya_printf("Short: %h\n", s);
    ya_printf("Long: %l\n", l);
    ya_printf("String: %s\n", str);
    ya_printf("Pointer: %p\n", ptr);
    ya_printf("IPv4: %4\n", ipv4);
    ya_printf("IPv6: %6\n", ipv6);
    ya_printf("MAC: %m\n", mac);

    // Memory dump of a portion of the main function's stack
    ya_printf("Memory dump: %M\n", &i, 64);

    // Register dump
    ya_printf("Register dump: %R\n");

   // Instruction dump
    ya_printf("Instruction dump: %I\n");
    return 0;
}

