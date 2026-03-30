#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to map relocation type to its name
const char *get_relocation_type_name(uint32_t type) {
    switch (type) {
        case R_X86_64_NONE: return "R_X86_64_NONE";
        case R_X86_64_64: return "R_X86_64_64";
        case R_X86_64_PC32: return "R_X86_64_PC32";
        case R_X86_64_GOT32: return "R_X86_64_GOT32";
        case R_X86_64_PLT32: return "R_X86_64_PLT32";
        case R_X86_64_COPY: return "R_X86_64_COPY";
        case R_X86_64_GLOB_DAT: return "R_X86_64_GLOB_DAT";
        case R_X86_64_JUMP_SLOT: return "R_X86_64_JUMP_SLOT";
        case R_X86_64_RELATIVE: return "R_X86_64_RELATIVE";
        case R_X86_64_GOTPCREL: return "R_X86_64_GOTPCREL";
        case R_X86_64_32: return "R_X86_64_32";
        case R_X86_64_32S: return "R_X86_64_32S";
        case R_X86_64_16: return "R_X86_64_16";
        case R_X86_64_PC16: return "R_X86_64_PC16";
        case R_X86_64_8: return "R_X86_64_8";
        case R_X86_64_PC8: return "R_X86_64_PC8";
        default: return "Unknown";
    }
}

// Function to dump relocation entries with symbol names and relocation type names
void dump_relocation_entries(int fd, Elf64_Shdr *shdr, const char *shstrtab, Elf64_Shdr *symtab_shdr, const char *strtab) {
    if (shdr->sh_type != SHT_REL && shdr->sh_type != SHT_RELA) {
        return;
    }

    printf("Relocation Section: %s\n", shstrtab + shdr->sh_name);

    // Seek to the start of the relocation entries
    if (lseek(fd, shdr->sh_offset, SEEK_SET) < 0) {
        perror("lseek");
        return;
    }

    int num_entries = shdr->sh_size / shdr->sh_entsize;

    for (int i = 0; i < num_entries; i++) {
        uint32_t sym_index;
        uint32_t rel_type;
        char

        sym_name[256] = {0};

        if (shdr->sh_type == SHT_RELA) {
            Elf64_Rela rela;
            if (read(fd, &rela, sizeof(rela)) != sizeof(rela)) {
                perror("read");
                return;
            }

            sym_index = ELF64_R_SYM(rela.r_info);
            rel_type = ELF64_R_TYPE(rela.r_info);
            snprintf(sym_name, sizeof(sym_name), "%s", &strtab[symtab_shdr->sh_offset + sym_index]);

            printf("  Relocation %d:\n", i);
            printf("    Offset: 0x%lx\n", rela.r_offset);
            printf("    Type:   %s (%u)\n", get_relocation_type_name(rel_type), rel_type);
            printf("    Symbol: %s (%u)\n", sym_name, sym_index);
            printf("    Addend: 0x%lx\n", rela.r_addend);

        } else if (shdr->sh_type == SHT_REL) {
            Elf64_Rel rel;
            if (read(fd, &rel, sizeof(rel)) != sizeof(rel)) {
                perror("read");
                return;
            }

            sym_index = ELF64_R_SYM(rel.r_info);
            rel_type = ELF64_R_TYPE(rel.r_info);
            snprintf(sym_name, sizeof(sym_name), "%s", &strtab[symtab_shdr->sh_offset + sym_index]);

            printf("  Relocation %d:\n", i);
            printf("    Offset: 0x%lx\n", rel.r_offset);
            printf("    Type:   %s (%u)\n", get_relocation_type_name(rel_type), rel_type);
            printf("    Symbol: %s (%u)\n", sym_name, sym_index);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ELF file>\n", argv[0]);
        return 1;
    }

    // Open the ELF file
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // Read the ELF header
    Elf64_Ehdr ehdr;
    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        perror("read");
        close(fd);
        return 1;
    }

    // Check if it's a valid ELF file
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not a valid ELF file\n");
        close(fd);
        return 1;
    }

    // Seek to the section headers
    if (lseek(fd, ehdr.e_shoff, SEEK_SET) < 0) {
        perror("lseek");
        close(fd);
        return 1;
    }

    // Allocate memory for section headers
    Elf64_Shdr *shdrs = malloc(ehdr.e_shnum * sizeof(Elf64_Shdr));
    if (!shdrs) {
        perror("malloc");
        close(fd);
        return 1;
    }

    // Read the section headers
    if (read(fd, shdrs, ehdr.e_shnum * sizeof(Elf64_Shdr)) != ehdr.e_shnum * sizeof(Elf64_Shdr)) {
        perror("read");
        free(shdrs);
        close(fd);
        return 1;
    }

    // Read the section header string table
    Elf64_Shdr *shstrtab_hdr = &shdrs[ehdr.e_shstrndx];
    char *shstrtab = malloc(shstrtab_hdr->sh_size);
    if (!shstrtab) {
        perror("malloc");
        free(shdrs);
        close(fd);
        return 1;
    }

    if (lseek(fd, shstrtab_hdr->sh_offset, SEEK_SET) < 0) {
        perror("lseek");
        free(shstrtab);
        free(shdrs);
        close(fd);
        return 1;
    }

    if (read(fd, shstrtab, shstrtab_hdr->sh_size) != shstrtab_hdr->sh_size) {
        perror("read");
        free(shstrtab);
        free(shdrs);
        close(fd);
        return 1;
    }

    // Locate the symbol table and associated string table
    Elf64_Shdr *symtab_shdr = NULL;
    char *strtab = NULL;

    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB || shdrs[i].sh_type == SHT_DYNSYM) {
            symtab_shdr = &shdrs[i];
            // Find the corresponding string table
            strtab = malloc(shdrs[symtab_shdr->sh_link].sh_size);
            if (!strtab) {
                perror("malloc");
                free(shstrtab);
                free(shdrs);
                close(fd);
                return 1;
            }

            if (lseek(fd, shdrs[symtab_shdr->sh_link].sh_offset, SEEK_SET) < 0) {
                perror("lseek");
                free(strtab);
                free(shstrtab);
                free(shdrs);
                close(fd);
                return 1;
            }

            if (read(fd, strtab, shdrs[symtab_shdr->sh_link].sh_size) != shdrs[symtab_shdr->sh_link].sh_size) {
                perror("read");
                free(strtab);
                free(shstrtab);
                free(shdrs);
                close(fd);
                return 1;
            }
            break;
        }
    }

    // Iterate over section headers to find relocation sections
    for (int i = 0; i < ehdr.e_shnum; i++) {
        dump_relocation_entries(fd, &shdrs[i], shstrtab, symtab_shdr, strtab);
    }

    // Free allocated memory
    free(strtab);
    free(shstrtab);
    free(shdrs);

    // Close the file
    close(fd);
    return 0;
}


