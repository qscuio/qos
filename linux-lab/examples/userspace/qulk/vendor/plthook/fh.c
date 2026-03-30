#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>
#include <link.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include "plthook.h"
#include "funchook.h"

#define MAX_SYMS 8192 * 2
#define MAX_SHDRS 256

#if 1
#define __TRACE() do {} while(0);
#else
#define __TRACE() syslog(1, "[%s][%d]-----\n", __func__, __LINE__)
#endif

struct syms 
{
	char *name;
	unsigned long type;
	unsigned long value;
	unsigned long size;
};

struct elf_section_range 
{
	char *sh_name;
	unsigned long sh_addr;
	unsigned int sh_size;
};

struct elf {
	ElfW(Ehdr) *ehdr;
	ElfW(Phdr) *phdr;
	ElfW(Shdr) *shdr;
	ElfW(Sym)  *sym;
	ElfW(Dyn)  *dyn;

	uint8_t *map;
	int map_size;
	char file[1024];
	size_t base_address;
	size_t code_address;
	size_t code_size;
	struct elf_section_range sh_range[MAX_SHDRS];
	struct syms lsyms[MAX_SYMS]; //local syms
	struct syms dsyms[MAX_SYMS]; //dynamic syms

	int lsc; //lsyms count
	int dsc; // dsyms count
	int shdr_count;

	char *StringTable;
	char *SymStringTable;
} current;

static int (*org_puts)(const char *str);
static int (*org_printf)(const char *restrict format, ... );
void *(*orig_malloc)(int size);
void (*orig_free)(void *ptr);

plthook_t *__plthook;
funchook_t *__funchook;
size_t __stack_base = 0;
char *find_symbol_name_by_addr(struct elf *e, size_t addr);
size_t find_symbol_addr_by_name(struct elf *e, const char *name);

#define MAX_STACK_DEPTH 10

#if 1
#define BUILD_CALL_STACK(buf)                                                          \
	do {                                                                               \
		int __j = 0;                                                                   \
		char *__n = NULL;                                                              \
		size_t __addr = 0;                                                             \
		__addr = (size_t)__builtin_return_address(0);                                  \
		if (__addr == 0)                                                               \
		break;                                                                         \
		__n = find_symbol_name_by_addr(&current, __addr);                              \
		if (__n != NULL) {                                                             \
			__j += sprintf(buf + __j, "%s", __n);                                      \
		} else {                                                                       \
			__j += sprintf(buf + __j, "%#lx", __addr);                                 \
		}                                                                              \
		__addr = (size_t)__builtin_return_address(1);                                  \
		if (__addr == 0)                                                               \
		break;                                                                         \
		__n = find_symbol_name_by_addr(&current, __addr);                              \
		if (__n != NULL) {                                                             \
			__j += sprintf(buf + __j, "<-%s", __n);                                    \
		} else {                                                                       \
			__j += sprintf(buf + __j, "<-%#lx", __addr);                               \
		}                                                                              \
		__addr = (size_t)__builtin_return_address(2);                                  \
		if (__addr == 0)                                                               \
		break;                                                                         \
		__n = find_symbol_name_by_addr(&current, __addr);                              \
		if (__n != NULL) {                                                             \
			__j += sprintf(buf + __j, "<-%s", __n);                                    \
		} else {                                                                       \
			__j += sprintf(buf + __j, "<-%#lx", __addr);                               \
		}                                                                              \
	} while (0)
#else
#define  BUILD_CALL_STACK(buf)                                                                          \
    do {                                                                                                \
        char *__name = NULL;                                                                            \
        char *__ptr = buf;                                                                              \
        uint32_t *__sp;                                                                                 \
        uint32_t *__lr;                                                                                 \
        uint32_t __depth = 0;                                                                           \
        size_t __prev = 0;                                                                              \
        size_t __ret = (size_t)__builtin_return_address(0);                                             \
                                                                                                        \
        asm (                                                                                           \
                "mov %0, sp\n"                                                                          \
                "mov %1, lr\n"                                                                          \
                : "=r" (__sp), "=r" (__lr)                                                              \
            );                                                                                          \
                                                                                                        \
        __name = find_symbol_name_by_addr(&current, (size_t)__ret);                                     \
        if (__name)                                                                                     \
        {                                                                                               \
            __ptr += printf(__ptr, "%s", __name);                                                       \
        }                                                                                               \
        else                                                                                            \
        {                                                                                               \
            __ptr += sprintf(__ptr, "%p", __ret);                                                       \
        }                                                                                               \
        __prev = __ret;                                                                                 \
                                                                                                        \
        for (__sp; __sp < (uint32_t *)__stack_base; __sp++)                                             \
        {                                                                                               \
            if (*__sp > current.code_address && *__sp < current.code_address + current.code_size)       \
            {                                                                                           \
                if (*__sp == __prev) continue;                                                          \
                __name = find_symbol_name_by_addr(&current, *__sp);                                     \
                if (__name)                                                                             \
                {                                                                                       \
                    __ptr += sprintf(__ptr, "<-%s", __name);                                            \
                }                                                                                       \
                else                                                                                    \
                {                                                                                       \
                    __ptr += sprintf(__ptr, "<-%p", *__sp);                                             \
                }                                                                                       \
                __prev == *__sp;                                                                        \
                if (__depth++ > 5) break;                                                               \
            }                                                                                           \
        }                                                                                               \
    } while (0)

#endif

char *find_symbol_name_by_addr(struct elf *e, size_t addr)
{
	int i = 0;

	if (addr == 0)
		return NULL;

	for (i = 0; i < e->lsc; i++)
	{
		if (addr > e->lsyms[i].value && addr < e->lsyms[i].value + e->lsyms[i].size)
			return e->lsyms[i].name;
	}

	return NULL;
}

size_t find_symbol_addr_by_name(struct elf *e, const char *name)
{
	int i = 0;

	if (name == NULL)
		return 0;

	for (i = 0; i < e->lsc; i++)
	{
		if (!strncmp(e->lsyms[i].name, name, strlen(name)))
			return e->lsyms[i].value;
	}

	return 0;
}


size_t get_stack_base_address(void)
{
    FILE *fp;
    char line[256];
    unsigned long start, end;
    char perm[5], dev[6], mapname[256];
    unsigned long offset, inode;
    size_t stack = 0;

    fp = fopen("/proc/self/maps", "r");
    if (fp == NULL) 
    {
	syslog(1, "Cannot open proc/self/maps\n");
	return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
	sscanf(line, "%lx-%lx %4s %lx %5s %ld %s",
		&start, &end, perm, &offset, dev, &inode, mapname);

	if (strstr(mapname, "[stack]")) {
	    stack = end;
	    break;
	}
    }

    fclose(fp);

    return stack;
}

void* hooked_malloc(int size)
{
	void *ptr = NULL;
	char buffer[512] = {'\0'};

	__TRACE();
	BUILD_CALL_STACK(buffer);

	ptr = (*orig_malloc)(size);
	syslog(1, "[+]  %p : %-8d by %s\n", ptr, size, buffer);

	return ptr;
}

void hooked_free(void *ptr)
{
	char buffer[512] = {'\0'};

	__TRACE();
	BUILD_CALL_STACK(buffer);

	(*orig_free)(ptr);
	syslog(1, "[-]  %p            by %s\n",  ptr, buffer);
}

struct sfoo {
	int x;
	int y;
};

int (*orig_foo_sfoo)(struct sfoo sf);

int hooked_foo_sfoo(struct sfoo sf) {
	int ret = 0;

	printf("[%s][%d] sf.x %d, sf.y %d\n", 
			__func__, __LINE__, sf.x, sf.y);

	ret = (*orig_foo_sfoo)(sf);


	printf("[%s][%d] sf.x %d, sf.y %d = %d\n", 
			__func__, __LINE__, sf.x, sf.y, ret);

	return ret;
}

static int puts_hook(const char *str)
{
	write(2, str, strlen(str));
	write(2, "\n", 1);  // `puts` automatically adds a newline
	return (*org_puts)(str);
}

static int printf_hook(const char *restrict format, ... )
{
	va_list args;
	va_start(args, format);

	// Create a buffer to hold the formatted string
	char buffer[1024];
	int length = vsnprintf(buffer, sizeof(buffer), format, args);

	// Ensure to terminate the variadic argument list
	va_end(args);

	// Write the formatted string to stderr
	write(2, buffer, length);

	// Call the original printf function with the same arguments
	va_start(args, format);
	int result = (*org_printf)(format, args);
	va_end(args);

	return result;
}

void elf_build_section_range(struct elf *e)
{
	ElfW(Ehdr) *ehdr;
	ElfW(Shdr) *shdr;

	char *StringTable;
	int i;

	e->shdr_count = 0;
	StringTable = e->StringTable;
	ehdr = e->ehdr;
	shdr = e->shdr;

	for (i = 0; i < ehdr->e_shnum; i++) {
		e->sh_range[i].sh_name = strdup(&StringTable[shdr[i].sh_name]);
		e->sh_range[i].sh_addr = shdr[i].sh_addr;
		e->sh_range[i].sh_size = shdr[i].sh_size;
		e->shdr_count++;
                if (!strncmp(e->sh_range[i].sh_name, ".text", strlen(".text")))
                {
                    e->code_address = e->sh_range[i].sh_addr;
                    e->code_size = e->sh_range[i].sh_size;
                }
	}

	return;
}

void elf_dump(struct elf *e)
{
	int i = 0;

	syslog(1, "    Local symbols(%d): \n", e->lsc);
	for (i = 0; i < e->lsc; i++)
	{
		syslog(1, "       name %s, addr: %lx, size: %ld\n", e->lsyms[i].name, e->lsyms[i].value, e->lsyms[i].size);
	}

	syslog(1, "    Dynamic symbols(%d): \n", e->dsc);
	for (i = 0; i < e->dsc; i++)
	{
		syslog(1, "       name %s, addr: %lx, size: %ld\n", e->dsyms[i].name, e->dsyms[i].value, e->dsyms[i].size);
	}

	syslog(1, "    Sections(%d): \n", e->shdr_count);
        for (i = 0; i < e->shdr_count; i++)
        {
		syslog(1, "       name %s, addr: %lx, size: %ld\n", e->sh_range[i].sh_name, e->sh_range[i].sh_addr, e->sh_range[i].sh_size);
        }
}

void elf_destroy(struct elf *e)
{
	int i = 0;

	if (e->map)
	{	
		munmap(e->map, e->map_size);
		e->map == NULL;
	}

	for (i = 0; i < e->lsc; i++)
	{
		free(e->lsyms[i].name);
		memset(&e->lsyms[i], 0, sizeof(e->lsyms[i]));
	}

	for (i = 0; i < e->dsc; i++)
	{
		free(e->dsyms[i].name);
		memset(&e->dsyms[i], 0, sizeof(e->dsyms[i]));
	}
}

int elf_build_sym_table(struct elf *e)
{
	unsigned int i, j, k;
	char *SymStrTable;
	ElfW(Ehdr) *ehdr;
	ElfW(Shdr) *shdr;
	ElfW(Sym) *symtab;
#ifdef __x86_64__
#define ELF_ST_TYPE ELF64_ST_TYPE
#else
#define ELF_ST_TYPE ELF32_ST_TYPE
#endif
	int st_type;

	e->lsc = 0;
	e->dsc = 0;

	ehdr = e->ehdr;
	shdr = e->shdr;

	for (i = 0; i < ehdr->e_shnum; i++) {
		if (shdr[i].sh_type == SHT_SYMTAB 
				|| shdr[i].sh_type == SHT_DYNSYM) {

			SymStrTable = (char *)&e->map[shdr[shdr[i].sh_link].sh_offset]; 
			symtab = (ElfW(Sym) *)&e->map[shdr[i].sh_offset];

			for (j = 0; j < shdr[i].sh_size / sizeof(ElfW(Sym)); j++, symtab++) {

				st_type = ELF_ST_TYPE(symtab->st_info);
				if (st_type != STT_FUNC)
					continue;

				//syslog(1, "%s, %lx, %ld\n", &SymStrTable[symtab->st_name], symtab->st_value, symtab->st_size);
				switch(shdr[i].sh_type) {
					case SHT_SYMTAB:
						e->lsyms[e->lsc].name = strdup(&SymStrTable[symtab->st_name]);
						e->lsyms[e->lsc].type = shdr[i].sh_type;
						if (ehdr->e_type == ET_EXEC)
							e->lsyms[e->lsc].value = symtab->st_value;
						else
							e->lsyms[e->lsc].value = e->base_address + symtab->st_value;
						e->lsyms[e->lsc].size = symtab->st_size;
						e->lsc++;
						break;
					case SHT_DYNSYM:
						e->dsyms[e->dsc].name = strdup(&SymStrTable[symtab->st_name]);
						e->dsyms[e->dsc].type = shdr[i].sh_type;
						if (ehdr->e_type == ET_EXEC)
							e->dsyms[e->dsc].value = symtab->st_value;
						else
							e->dsyms[e->dsc].value = e->base_address + symtab->st_value;
						e->dsyms[e->dsc].size = symtab->st_size;
						e->dsc++;
						break;
				}
			}
		}
	}

	e->StringTable = (char *)&e->map[shdr[ehdr->e_shstrndx].sh_offset];
	for (i = 0; i < ehdr->e_shnum; i++) {
		if (!strcmp(&e->StringTable[shdr[i].sh_name], ".plt.sec")) {
			// syslog(1, "shdr[i].sh_size %ld, %lx\n", shdr[i].sh_size, shdr[i].sh_addr);

			/* Skip first one. */
			for (k = 1, j = 0; j < shdr[i].sh_size; j += 16) {
				e->dsyms[k++].value = shdr[i].sh_addr + j; 
			}
			break;
		}
	} 

	return 0;
}

void elf_init(void)
{
	int fd = 0;
	int ret = 0;
	struct stat st = {0};

	ret = readlink("/proc/self/exe", current.file, sizeof(current.file)-1);
	if (ret == 0)
	{
		syslog(1, "Cannot get current process binary\n");
		return;
	}

	printf("Current process %s\n\n", current.file);

	if ((fd = open(current.file, O_RDONLY)) < 0) {
		syslog(1, "Unable to open %s: %s\n", current.file, strerror(errno));
		return;
	}

	if (fstat(fd, &st) < 0) {
		syslog(1, "fstat");
		return;
	}

	current.map = (uint8_t *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (current.map == MAP_FAILED) {
		syslog(1, "mmap");
		return;
	}
	current.map_size = st.st_size;

	current.ehdr = (ElfW(Ehdr) *)current.map;
	current.shdr = (ElfW(Shdr) *)(current.map + current.ehdr->e_shoff);
	current.phdr = (ElfW(Phdr) *)(current.map + current.ehdr->e_phoff);

	current.StringTable = (char *)&current.map[current.shdr[current.ehdr->e_shstrndx].sh_offset];

	if (current.ehdr->e_shnum > 0 && current.ehdr->e_shstrndx != SHN_UNDEF)
		elf_build_section_range(&current);

	elf_build_sym_table(&current);

	elf_dump(&current);
}

void register_plt_hook(void)
{
	int ret = 0;

	if (plthook_open(&__plthook, NULL) != 0) {
		syslog(1, "plthook_open error: %s\n", plthook_error());
		return;
	}

	if (plthook_replace(__plthook, "puts", (void*)puts_hook, (void *)&org_puts) != 0) {
		printf("plthook_replace error: %s\n", plthook_error());
	}

#if 0
	if (plthook_replace(__plthook, "printf", (void*)printf_hook, (void *)&org_printf) != 0) {
		printf("plthook_replace error: %s\n", plthook_error());
	}
#endif

	ret = plthook_replace(__plthook, "malloc", (void *)hooked_malloc, (void *)&orig_malloc);
	if (ret != 0)
	{
		syslog(1, "failed to hook malloc\n");
		return;
	}

	ret = plthook_replace(__plthook, "free", (void *)hooked_free, (void *)&orig_free);
	if (ret != 0)
	{
		syslog(1, "failed to hook free\n");
		return;
	}

	plthook_close(__plthook);

        syslog (1, "malloc/free is hooked\n");

	return;
}

void unregister_plt_hook(void)
{
	int ret = 0;

	if (plthook_open(&__plthook, NULL) != 0) {
		syslog(1, "plthook_open error: %s\n", plthook_error());
		return;
	}

	if (plthook_replace(__plthook, "puts", (void*)org_puts, NULL) != 0) {
		printf("plthook_replace error: %s\n", plthook_error());
	}

#if 0
	if (plthook_replace(__plthook, "printf", (void*)&org_printf, NULL) != 0) {
		printf("plthook_replace error: %s\n", plthook_error());
	}
#endif

	ret = plthook_replace(__plthook, "malloc", (void *)orig_malloc, NULL);
	if (ret != 0)
	{
		syslog(1, "failed to unhook malloc\n");
		return;
	}

	ret = plthook_replace(__plthook, "free", (void *)orig_free, NULL);
	if (ret != 0)
	{
		syslog(1, "failed to unhook free\n");
		return;
	}

	elf_destroy(&current);

	plthook_close(__plthook);

	return;
}

void register_func_hook(void)
{
	size_t addr = 0;

	funchook_set_debug_file("funchook.log");
    __funchook = funchook_create();

	/* Stripped binary cannot use this method. */
	orig_foo_sfoo = find_symbol_addr_by_name(&current, "foo_sfoo");
	if (orig_foo_sfoo == 0)
	{
		printf("Cannot find function foo_sfoo\n");
		return;
	}

	printf("[%s][%d] foo_sfoo %p\n", __func__, __LINE__, orig_foo_sfoo);
	funchook_prepare(__funchook, (void **)&orig_foo_sfoo, hooked_foo_sfoo);
	funchook_install(__funchook, 0);
}

void unregister_func_hook(void)
{
	funchook_uninstall(__funchook, 0);
    funchook_destroy(__funchook);
}

void __attribute__((constructor(1000))) fh_ctor(void)
{
    __stack_base = get_stack_base_address();

	elf_init();

	register_plt_hook();
	register_func_hook();
}

void __attribute__((destructor)) fh_dtor(void)
{
	unregister_func_hook();
	unregister_plt_hook();
	printf("Hooking unloaded on dlclose()\n");
}
