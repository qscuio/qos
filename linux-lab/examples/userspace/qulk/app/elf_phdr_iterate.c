#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    // Seek to the program headers
    if (lseek(fd, ehdr.e_phoff, SEEK_SET) < 0) {
        perror("lseek");
        close(fd);
        return 1;
    }

    // Iterate over program headers
    for (int i = 0; i < ehdr.e_phnum; i++) {
        Elf64_Phdr phdr;
        if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
            perror("read");
            close(fd);
            return 1;
        }

        // Print information about the program header
        printf("Program Header %d:\n", i);
        printf("  Type:   0x%x\n", phdr.p_type);
        printf("  Offset: 0x%lx\n", phdr.p_offset);
        printf("  VAddr:  0x%lx\n", phdr.p_vaddr);
        printf("  PAddr:  0x%lx\n", phdr.p_paddr);
        printf("  Filesz: 0x%lx\n", phdr.p_filesz);
        printf("  Memsz:  0x%lx\n", phdr.p_memsz);
        printf("  Flags:  0x%x\n", phdr.p_flags);
        printf("  Align:  0x%lx\n", phdr.p_align);
    }

    // Close the file
    close(fd);
    return 0;
}

