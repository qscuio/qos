#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <link.h>
#include <stdbool.h>

#define MAX_REL_TYPES 256

// Function to open the ELF file
Elf64_Ehdr* open_elf(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    fstat(fd, &st);

    Elf64_Ehdr *ehdr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ehdr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return ehdr;
}

// Use a simple array as a set to store seen dependencies
#define MAX_DEPENDENCIES 100
char seen_dependencies[MAX_DEPENDENCIES][256];
int seen_count = 0;

bool is_dependency_seen(const char *dep) {
    for (int i = 0; i < seen_count; i++) {
        if (strcmp(seen_dependencies[i], dep) == 0) {
            return true;
        }
    }
    return false;
}

void add_dependency(const char *dep) {
    if (seen_count < MAX_DEPENDENCIES) {
        strncpy(seen_dependencies[seen_count], dep, 256);
        seen_count++;
    }
}

void find_dependencies_recursive(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Failed to stat file");
        close(fd);
        return;
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ehdr == MAP_FAILED) {
        perror("Failed to mmap file");
        close(fd);
        return;
    }

    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)ehdr + ehdr->e_phoff);
    const char *strtab = NULL;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            Elf64_Dyn *dyn = (Elf64_Dyn *)((char *)ehdr + phdr[i].p_offset);
            while (dyn->d_tag != DT_NULL) {
                if (dyn->d_tag == DT_STRTAB) {
                    strtab = (const char *)ehdr + dyn->d_un.d_ptr;
                }
                dyn++;
            }
            break;
        }
    }

    if (!strtab) {
        printf("String table not found for %s!\n", filename);
        munmap(ehdr, st.st_size);
        close(fd);
        return;
    }

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            Elf64_Dyn *dyn = (Elf64_Dyn *)((char *)ehdr + phdr[i].p_offset);
            while (dyn->d_tag != DT_NULL) {
                if (dyn->d_tag == DT_NEEDED) {
                    const char *dependency = strtab + dyn->d_un.d_val;
                    if (!is_dependency_seen(dependency)) {
                        printf("Dependency: %s\n", dependency);
                        add_dependency(dependency);
                        // Recurse to find the dependencies of this dependency
                        char dep_path[256];
                        snprintf(dep_path, sizeof(dep_path), "/lib/x86_64-linux-gnu/%s", dependency);
                        find_dependencies_recursive(dep_path);
                    }
                }
                dyn++;
            }
            break;
        }
    }

    munmap(ehdr, st.st_size);
    close(fd);
}

void find_dependencies(Elf64_Ehdr *ehdr) {
    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)ehdr + ehdr->e_phoff);
    const char *strtab = NULL;

    // First, locate the string table
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            Elf64_Dyn *dyn = (Elf64_Dyn *)((char *)ehdr + phdr[i].p_offset);
            while (dyn->d_tag != DT_NULL) {
                if (dyn->d_tag == DT_STRTAB) {
                    strtab = (const char *)ehdr + dyn->d_un.d_ptr;
                }
                dyn++;
            }
        }
    }

    if (!strtab) {
        printf("String table not found!\n");
        return;
    }

    // Now, find dependencies
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            Elf64_Dyn *dyn = (Elf64_Dyn *)((char *)ehdr + phdr[i].p_offset);
            while (dyn->d_tag != DT_NULL) {
                if (dyn->d_tag == DT_NEEDED) {
                    printf("Dependency: %s\n", strtab + dyn->d_un.d_val);
                }
                dyn++;
            }
        }
    }
}

void iterate_program_headers(Elf64_Ehdr *ehdr) {
    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)ehdr + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        switch (phdr[i].p_type) {
            case PT_NULL: printf("Program Header %d: Type = NULL\n", i); break;
            case PT_LOAD: printf("Program Header %d: Type = LOAD\n", i); break;
            case PT_DYNAMIC: printf("Program Header %d: Type = DYNAMIC\n", i); break;
            case PT_INTERP: printf("Program Header %d: Type = INTERP\n", i); break;
            case PT_NOTE: printf("Program Header %d: Type = NOTE\n", i); break;
            case PT_SHLIB: printf("Program Header %d: Type = SHLIB\n", i); break;
            case PT_PHDR: printf("Program Header %d: Type = PHDR\n", i); break;
            case PT_TLS: printf("Program Header %d: Type = TLS\n", i); break;
            case 0x6474e550: printf("Program Header %d: Type = GNU_EH_FRAME\n", i); break;
            case 0x6474e551: printf("Program Header %d: Type = GNU_STACK\n", i); break;
            case 0x6474e552: printf("Program Header %d: Type = GNU_RELRO\n", i); break;
            case 0x6474e553: printf("Program Header %d: Type = GNU_PROPERTY\n", i); break;
            default: printf("Program Header %d: Type = 0x%x\n", i, phdr[i].p_type); break;
        }
        printf("  Offset = 0x%lx\n", phdr[i].p_offset);
    }
}

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
        case R_X86_64_DTPMOD64: return "R_X86_64_DTPMOD64";
        case R_X86_64_DTPOFF64: return "R_X86_64_DTPOFF64";
        case R_X86_64_TPOFF64: return "R_X86_64_TPOFF64";
        case R_X86_64_TLSGD: return "R_X86_64_TLSGD";
        case R_X86_64_TLSLD: return "R_X86_64_TLSLD";
        case R_X86_64_DTPOFF32: return "R_X86_64_DTPOFF32";
        case R_X86_64_GOTTPOFF: return "R_X86_64_GOTTPOFF";
        case R_X86_64_TPOFF32: return "R_X86_64_TPOFF32";
        case R_X86_64_PC64: return "R_X86_64_PC64";
        case R_X86_64_GOTOFF64: return "R_X86_64_GOTOFF64";
        case R_X86_64_GOTPC32: return "R_X86_64_GOTPC32";
        case R_X86_64_SIZE32: return "R_X86_64_SIZE32";
        case R_X86_64_SIZE64: return "R_X86_64_SIZE64";
        case R_X86_64_GOTPC32_TLSDESC: return "R_X86_64_GOTPC32_TLSDESC";
        case R_X86_64_TLSDESC_CALL: return "R_X86_64_TLSDESC_CALL";
        case R_X86_64_TLSDESC: return "R_X86_64_TLSDESC";
        case R_X86_64_IRELATIVE: return "R_X86_64_IRELATIVE";
        case R_X86_64_RELATIVE64: return "R_X86_64_RELATIVE64";
        case R_X86_64_GOTPCRELX: return "R_X86_64_GOTPCRELX";
        case R_X86_64_REX_GOTPCRELX: return "R_X86_64_REX_GOTPCRELX";
        default: return "(unknown)";
    }
}

void dump_relocations(const Elf64_Shdr *shdr, const Elf64_Ehdr *ehdr) {
    Elf64_Shdr *section_headers = (Elf64_Shdr *)((char *)ehdr + ehdr->e_shoff);
    const char *section_strtab = (const char *)ehdr + section_headers[ehdr->e_shstrndx].sh_offset;

    Elf64_Shdr *symtab_section = &section_headers[shdr->sh_link];
    const Elf64_Sym *symtab = (Elf64_Sym *)((char *)ehdr + symtab_section->sh_offset);
    const char *strtab = (const char *)ehdr + section_headers[symtab_section->sh_link].sh_offset;

    if (shdr->sh_type == SHT_RELA) {
        Elf64_Rela *rela = (Elf64_Rela *)((char *)ehdr + shdr->sh_offset);
        for (int i = 0; i < shdr->sh_size / sizeof(Elf64_Rela); i++) {
            uint32_t r_type = ELF64_R_TYPE(rela[i].r_info);
            uint32_t r_sym = ELF64_R_SYM(rela[i].r_info);
            const char *sym_name = r_sym ? strtab + symtab[r_sym].st_name : "(None)";
            printf("Relocation: Type = %s, Symbol = %s\n", get_relocation_type_name(r_type), sym_name);
        }
    } else if (shdr->sh_type == SHT_REL) {
        Elf64_Rel *rel = (Elf64_Rel *)((char *)ehdr + shdr->sh_offset);
        for (int i = 0; i < shdr->sh_size / sizeof(Elf64_Rel); i++) {
            uint32_t r_type = ELF64_R_TYPE(rel[i].r_info);
            uint32_t r_sym = ELF64_R_SYM(rel[i].r_info);
            const char *sym_name = r_sym ? strtab + symtab[r_sym].st_name : "(None)";
            printf("Relocation: Type = %s, Symbol = %s\n", get_relocation_type_name(r_type), sym_name);
        }
    } else {
        printf("Warning: Unsupported relocation section type %u\n", shdr->sh_type);
    }
}

void find_dependencies_and_relocations(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Failed to stat file");
        close(fd);
        return;
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ehdr == MAP_FAILED) {
        perror("Failed to mmap file");
        close(fd);
        return;
    }

    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)ehdr + ehdr->e_phoff);
    const char *strtab = NULL;

    // Find and process dynamic section for dependencies
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            Elf64_Dyn *dyn = (Elf64_Dyn *)((char *)ehdr + phdr[i].p_offset);
            while (dyn->d_tag != DT_NULL) {
                if (dyn->d_tag == DT_STRTAB) {
                    strtab = (const char *)ehdr + dyn->d_un.d_ptr;
                }
                dyn++;
            }
            break;
        }
    }

    if (!strtab) {
        printf("String table not found for %s!\n", filename);
        munmap(ehdr, st.st_size);
        close(fd);
        return;
    }

    // Find dependencies
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            Elf64_Dyn *dyn = (Elf64_Dyn *)((char *)ehdr + phdr[i].p_offset);
            while (dyn->d_tag != DT_NULL) {
                if (dyn->d_tag == DT_NEEDED) {
                    const char *dependency = strtab + dyn->d_un.d_val;
                    if (!is_dependency_seen(dependency)) {
                        printf("Dependency: %s\n", dependency);
                        add_dependency(dependency);
                        // Recurse to find the dependencies of this dependency
                        char dep_path[256];
                        snprintf(dep_path, sizeof(dep_path), "/lib/x86_64-linux-gnu/%s", dependency);
                        find_dependencies_and_relocations(dep_path);
                    }
                }
                dyn++;
            }
            break;
        }
    }

    // Find and dump relocations
    Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)ehdr + ehdr->e_shoff);
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_RELA || shdr[i].sh_type == SHT_REL) {
            dump_relocations(&shdr[i], ehdr);
        }
    }

    munmap(ehdr, st.st_size);
    close(fd);
}

// Function to dump relocation entries
void dump_relocations2(Elf64_Ehdr *ehdr) {
    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)ehdr + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            Elf64_Dyn *dyn = (Elf64_Dyn *)((char *)ehdr + phdr[i].p_offset);
            Elf64_Rela *rela = NULL;
            size_t rela_size = 0;

            while (dyn->d_tag != DT_NULL) {
                if (dyn->d_tag == DT_RELA) {
                    rela = (Elf64_Rela *)((char *)ehdr + dyn->d_un.d_ptr);
                } else if (dyn->d_tag == DT_RELASZ) {
                    rela_size = dyn->d_un.d_val;
                }
                dyn++;
            }

            if (rela) {
                size_t n = rela_size / sizeof(Elf64_Rela);
                for (size_t i = 0; i < n; i++) {
                    printf("Relocation %zu: Offset = 0x%lx, Type = %ld\n", i, rela[i].r_offset, ELF64_R_TYPE(rela[i].r_info));
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <elf_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Elf64_Ehdr *ehdr = open_elf(argv[1]);
    iterate_program_headers(ehdr);
	//find_dependencies_recursive(argv[1]);
	find_dependencies_and_relocations(argv[1]);

    munmap(ehdr, sizeof(*ehdr));
    return EXIT_SUCCESS;
}

