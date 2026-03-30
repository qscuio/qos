/*
 * Copyright (C) 2006  Trevor Woerner
 */

// objdump -t userspace/app/binaries/elf_sections
// objdump -h userspace/app/binaries/elf_sections
// objdump -d -j my_code_section_sub userspace/app/binaries/elf_sections
//
// userspace/app/binaries/elf_sections:     file format elf64-x86-64
//
//
// Disassembly of section my_code_section_sub:
//
//  0000000000001210 <sub>:
// 1210:       f3 0f 1e fa             endbr64 
// 1214:       55                      push   %rbp
// 1215:       48 89 e5                mov    %rsp,%rbp
// 1218:       89 7d fc                mov    %edi,-0x4(%rbp)
// 121b:       89 75 f8                mov    %esi,-0x8(%rbp)
// 121e:       8b 45 fc                mov    -0x4(%rbp),%eax
// 1221:       2b 45 f8                sub    -0x8(%rbp),%eax
// 1224:       5d                      pop    %rbp
// 1225:       c3                      ret    
// readelf -S userspace/app/binaries/elf_sections

#include <stdio.h>

int add (int, int) __attribute__ ((section ("my_code_section_add")));
int sub (int, int) __attribute__ ((section ("my_code_section_sub")));
int global_val     __attribute__ ((section ("my_data_section_gblobal")));
int gval_init      __attribute__ ((section ("my_data_section_init"))) = 29;

int add (int i, int j)
{
    return i+j;
}
int sub (int i, int j)
{
    return i-j;
}

int main (void)
{
    int local_val = 25;
    global_val = 17;

    printf ("local_val: %d    global_val: %d    gval_init: %d\n",
	    local_val, global_val, gval_init);
    printf ("%d + %d = %d\n", local_val, global_val,
	    add (local_val, global_val));

    printf ("%d - %d = %d\n", local_val, global_val,
	    sub (local_val, global_val));

    return 0;
}
