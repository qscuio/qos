#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libelf.h>
#include <gelf.h>

typedef struct {
    unsigned long address;
    size_t size;
} FunctionInfo;

FunctionInfo get_function_info(const char *filename, const char *function_name) {
    FunctionInfo func_info = {0, 0};

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library initialization failed: %s\n", elf_errmsg(-1));
        return func_info;
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        return func_info;
    }

    Elf *elf = elf_begin(fileno(file), ELF_C_READ, NULL);
    if (!elf) {
        fprintf(stderr, "elf_begin() failed: %s\n", elf_errmsg(-1));
        fclose(file);
        return func_info;
    }

    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            fprintf(stderr, "gelf_getshdr() failed: %s\n", elf_errmsg(-1));
            continue;
        }

        if (shdr.sh_type == SHT_SYMTAB) {
            Elf_Data *data = elf_getdata(scn, NULL);
            if (!data) {
                fprintf(stderr, "elf_getdata() failed: %s\n", elf_errmsg(-1));
                continue;
            }

            int count = shdr.sh_size / shdr.sh_entsize;
            for (int i = 0; i < count; i++) {
                GElf_Sym sym;
                if (gelf_getsym(data, i, &sym) != &sym) {
                    fprintf(stderr, "gelf_getsym() failed: %s\n", elf_errmsg(-1));
                    continue;
                }

                if (strcmp(function_name, elf_strptr(elf, shdr.sh_link, sym.st_name)) == 0) {
                    func_info.address = sym.st_value;
                    func_info.size = sym.st_size;
                    elf_end(elf);
                    fclose(file);
                    return func_info;
                }
            }
        }
    }

    elf_end(elf);
    fclose(file);
    fprintf(stderr, "Function '%s' not found in %s\n", function_name, filename);
    return func_info;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <function_name>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    const char *function_name = argv[2];

    FunctionInfo info = get_function_info(filename, function_name);

    if (info.address != 0 && info.size != 0) {
        printf("Function '%s' found at address 0x%lx with size %zu bytes\n",
               function_name, info.address, info.size);
    } else {
        printf("Function '%s' not found in %s\n", function_name, filename);
    }

    return 0;
}

