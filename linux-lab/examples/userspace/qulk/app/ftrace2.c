/*
 * ftrace (Function trace) local execution tracing 
 * <Ryan.Oneill@LeviathanSecurity.com>
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <elf.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/reg.h>
#include <stdarg.h>
#include <syslog.h>
#include <signal.h>
#include "compiler.h"
#include "hash.h"
#include "hmap.h"
#include "util.h"

/*
 * For our color coding output
 */
#define WHITE "\x1B[37m"
#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define DEFAULT_COLOR  "\x1B[0m"

#define MAX_SYMS 8192 * 2
#define MAX_SHDRS 256

/*
 * On 32bit systems should be set:
 * export FTRACE_ARCH=32
 */
#define FTRACE_ENV "FTRACE_ARCH"

#define MAX_ADDR_SPACE 256 
#define MAXSTR 512

#define TEXT_SPACE  0
#define DATA_SPACE  1
#define STACK_SPACE 2
#define HEAP_SPACE  3

#define CALLSTACK_DEPTH 0xf4240
#define BUFFER_SIZE 256

#define SYM_TYPE_LOCAL
#define SYM_TYPE_SHARED

struct syms 
{
	char *name;
	unsigned long type;
	unsigned long value;
};

enum {
	MA_TYPE_NONE,
	MA_TYPE_CODE,
	MA_TYPE_DATA,
	MA_TYPE_HEAP,
	MA_TYPE_STACK,
};

char *ma_type[] = {
	"MA_TYPE_NONE",
	"MA_TYPE_CODE",
	"MA_TYPE_DATA",
	"MA_TYPE_HEAP",
	"MA_TYPE_STACK",
};

struct memory_area
{
	size_t svaddr;
	size_t evaddr;
	size_t size;
	size_t type;
	char perms[128];
};

struct branch_instr {
	char *mnemonic;
	uint8_t opcode;
};

typedef struct breakpoint {
	unsigned long vaddr;
	long orig_code;
} breakpoint_t;

typedef struct calldata {
	char *symname;
	char *string;
	unsigned long vaddr;
	unsigned long retaddr;
	//	unsigned int depth;
	breakpoint_t breakpoint;
} calldata_t;

typedef struct callstack {
	calldata_t *calldata;
	unsigned int depth; 
} callstack_t;

struct call_list {
	char *callstring;
	struct call_list *next;
};

struct elf_section_range 
{
	char *sh_name;
	unsigned long sh_addr;
	unsigned int sh_size;
};

#ifdef __x86_64__
struct elf {
	struct hmap_node node;
	Elf64_Ehdr *ehdr;
	Elf64_Phdr *phdr;
	Elf64_Shdr *shdr;
	Elf64_Sym  *sym;
	Elf64_Dyn  *dyn;

	uint8_t *map;
	char file[256];
	int nma;       /* number of memore area */
	size_t hash;
	size_t base_address;
	struct memory_area mas[128];
	struct elf_section_range sh_range[MAX_SHDRS];
	struct syms lsyms[MAX_SYMS]; //local syms
	struct syms dsyms[MAX_SYMS]; //dynamic syms

	int lsc; //lsyms count
	int dsc; // dsyms count
	int shdr_count;

	char *StringTable;
	char *SymStringTable;
};
#else
struct elf {
	struct hmap_node node;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	Elf32_Shdr *shdr;
	Elf32_Sym  *sym;
	Elf32_Dyn  *dyn;

	uint8_t *map;
	char file[256];
	int nma;        /* number of memory aread */
	size_t hash;
	size_t base_address;
	struct memory_area mas[128];

	struct elf_section_range sh_range[MAX_SHDRS];
	struct syms lsyms[MAX_SYMS]; //local syms
	struct syms dsyms[MAX_SYMS]; //dynamic syms
	int lsc; //lsyms count
	int dsc; // dsyms count
	int shdr_count;

	char *StringTable;
	char *SymStringTable;
};
#endif

struct process
{
	char *path;
	char maps[128];
	pid_t pid;
	size_t hash;
	struct hmap *elfs;
	callstack_t callstack;
	int attached;
};

#define BRANCH_INSTR_LEN_MAX 5

/*
 * Table for (non-call) branch instructions used 
 * in our control flow analysis.
 */
struct branch_instr branch_table[64] = {
	{"jo",  0x70}, 
	{"jno", 0x71},  {"jb", 0x72},  {"jnae", 0x72},  {"jc", 0x72},  {"jnb", 0x73},
	{"jae", 0x73},  {"jnc", 0x73}, {"jz", 0x74},    {"je", 0x74},  {"jnz", 0x75},
	{"jne", 0x75},  {"jbe", 0x76}, {"jna", 0x76},   {"jnbe", 0x77}, {"ja", 0x77},
	{"js",  0x78},  {"jns", 0x79}, {"jp", 0x7a},	{"jpe", 0x7a}, {"jnp", 0x7b},
	{"jpo", 0x7b},  {"jl", 0x7c},  {"jnge", 0x7c},  {"jnl", 0x7d}, {"jge", 0x7d},
	{"jle", 0x7e},  {"jng", 0x7e}, {"jnle", 0x7f},  {"jg", 0x7f},  {"jmp", 0xeb},
	{"jmp", 0xe9},  {"jmpf", 0xea}, {NULL, 0}
};

struct process GP = {0};

static int interrupted = 0;

void callstack_flush(struct process *p);

void ft_process_signal_handler(int sig) {
	interrupted = 1;
	//printf("Caught signal %d(%s)\n", sig, strsignal(sig));
}

void *HeapAlloc(unsigned int len)
{
	uint8_t *mem = malloc(len);
	if (!mem) {
		perror("malloc");
		exit(-1);
	}
	return mem;
}

char *xstrdup(const char *s)
{
	char *p = strdup(s);
	if (p == NULL) {
		perror("strdup");
		exit(-1);
	}
	return p;
}

char *xfmtstrdup(char *fmt, ...)
{
	char *s, buf[512];
	va_list va;

	va_start (va, fmt);
	vsnprintf (buf, sizeof(buf), fmt, va);
	s = xstrdup(buf);

	return s;
}

void set_breakpoint(struct process *p)
{
	int status;
	long orig = ptrace(PTRACE_PEEKTEXT, p->pid, p->callstack.calldata[p->callstack.depth].retaddr);
	long trap;

	trap = (orig & ~0xff) | 0xcc;

	//printf("[+] Setting breakpoint on 0x%lx\n", p->callstack.calldata[p->callstack.depth].retaddr);

	ptrace(PTRACE_POKETEXT, p->pid, p->callstack.calldata[p->callstack.depth].retaddr, trap);
	p->callstack.calldata[p->callstack.depth].breakpoint.orig_code = orig;
	p->callstack.calldata[p->callstack.depth].breakpoint.vaddr = p->callstack.calldata[p->callstack.depth].retaddr;

}

void remove_breakpoint(struct process *p)
{
	int status;

	//printf("[+] Removing breakpoint from 0x%lx\n", p->callstack.calldata[p->callstack.depth].retaddr);

	ptrace(PTRACE_POKETEXT, p->pid, 
			p->callstack.calldata[p->callstack.depth].retaddr, p->callstack.calldata[p->callstack.depth].breakpoint.orig_code);
}

/*
 * Simple array implementation of stack
 * to keep track of function depth and return values
 */

void callstack_init(struct process *p)
{
	p->callstack.calldata = (calldata_t *)HeapAlloc(sizeof(calldata_t) * CALLSTACK_DEPTH);
	p->callstack.depth = -1; // 0 is first element

}

void callstack_push(struct process *p, calldata_t *calldata)
{
	if (interrupted)
		return;

	memcpy(&p->callstack.calldata[++p->callstack.depth], calldata, sizeof(calldata_t));
	set_breakpoint(p);
}

calldata_t *callstack_pop(struct process *p)
{
	if (p->callstack.depth == -1) 
		return NULL;

	remove_breakpoint(p);
	return &p->callstack.calldata[p->callstack.depth--];
}

/* View the top of the stack without popping */
calldata_t *callstack_peek(struct process *p)
{
	if (p->callstack.depth == -1)
		return NULL;

	return &p->callstack.calldata[p->callstack.depth];
}

void callstack_flush(struct process *p)
{
	while(callstack_peek(p))
		callstack_pop(p);
}

struct call_list *add_call_string(struct call_list **head, const char *string)
{
	struct call_list *tmp = (struct call_list *)HeapAlloc(sizeof(struct call_list));

	tmp->callstring = (char *)xstrdup(string);
	tmp->next = *head; 
	*head = tmp;

	return *head;
}

void clear_call_list(struct call_list **head)
{
	struct call_list *tmp;

	if (!head)
		return;

	while (*head != NULL) {
		tmp = (*head)->next;
		free (*head);
		*head = tmp;
	}
}

struct branch_instr * search_branch_instr(uint8_t instr)
{
	int i;
	struct branch_instr *p, *ret;

	for (i = 0, p = branch_table; p->mnemonic != NULL; p++, i++) {
		if (instr == p->opcode)
			return p;
	}

	return NULL;
}

/*
 * ptrace functions
 */
int pid_read(int pid, void *dst, const void *src, size_t len)
{
	int sz = len / sizeof(void *);
	int rem = len % sizeof(void *);
	unsigned char *s = (unsigned char *)src;
	unsigned char *d = (unsigned char *)dst;
	long word;

	while (sz-- != 0) {
		if (interrupted)
			return -1;

		word = ptrace(PTRACE_PEEKTEXT, pid, s, NULL);
		if (word == -1 && errno) 
			return -1;

		*(long *)d = word;
		s += sizeof(long);
		d += sizeof(long);
	}

	return 0;
}

void ft_process_signal_init(struct process *p)
{
	signal(SIGINT, ft_process_signal_handler);
}

void ft_process_dump(struct process *p)
{
	int i = 0;
	struct elf *node = NULL;

	printf("Process: %s\n", p->path);
	printf("  pid: %d\n", p->pid);
	printf("  maps: %s\n", p->maps);
	printf("  maps: %s\n", p->maps);
	printf("  hash: %lx\n", p->hash);
	printf("  elfs (%ld):\n", hmap_count(p->elfs));

	HMAP_FOR_EACH (node, node, p->elfs) {
		printf ("   File: %s\n", node->file);
		for (i = 0; i < node->nma; i++)
		{
			printf("       svaddr %lx, evaddr %lx, perms %s, type %ld(%s)\n", 
					node->mas[i].svaddr, node->mas[i].evaddr, node->mas[i].perms, node->mas[i].type, ma_type[node->mas[i].type]);
		}
		printf("    Sections(%d): \n", node->shdr_count);
		for (i = 0; i < node->shdr_count; i++)
		{
			printf("       name %s, addr: %lx, size %d(%x)\n", node->sh_range[i].sh_name, node->sh_range[i].sh_addr, node->sh_range[i].sh_size, node->sh_range[i].sh_size);
		}

		printf("    Local symbols(%d): \n", node->lsc);
		for (i = 0; i < node->lsc; i++)
		{
			printf("       name %s, addr: %lx\n", node->lsyms[i].name, node->lsyms[i].value);
		}

		printf("    Dynamic symbols(%d): \n", node->dsc);
		for (i = 0; i < node->dsc; i++)
		{
			printf("       name %s, addr: %lx\n", node->dsyms[i].name, node->dsyms[i].value);
		}
	}

	return;
}

/*
 * A couple of commonly used utility
 * functions for mem allocation
 * malloc, strdup wrappers.
 */
__maybe_unused static size_t elf_hash_by_name(char *name)
{
	return hash_string(name, 0);
}

int ft_pid_read(int pid, void *dst, const void *src, size_t len)
{
	int sz = len / sizeof(void *);
	int rem = len % sizeof(void *);
	unsigned char *s = (unsigned char *)src;
	unsigned char *d = (unsigned char *)dst;
	long word;

	while (sz-- != 0) {
		word = ptrace(PTRACE_PEEKTEXT, pid, s, NULL);
		if (word == -1 && errno) 
			return -1;

		*(long *)d = word;
		s += sizeof(long);
		d += sizeof(long);
	}

	return 0;
}

struct elf* ft_process_get_self(struct process *p)
{
	return (struct elf *)hmap_first_with_hash(p->elfs, p->hash);
}
/*
 * SHT_NULL: 表示此段头表项未使用。
 * SHT_PROGBITS: 表示此段包含程序定义的信息。
 * SHT_SYMTAB: 表示符号表，通常用于链接过程。
 * SHT_STRTAB: 表示字符串表，包含零终止的字符串。
 * SHT_RELA: 表示带有显式附加项的重定位表。
 * SHT_HASH: 表示符号散列表。
 * SHT_DYNAMIC: 表示动态链接信息。
 * SHT_NOTE: 表示附注信息。
 * SHT_NOBITS: 表示此段不占用文件中的空间。
 * SHT_REL: 表示不带显式附加项的重定位表。
 * SHT_SHLIB: 保留值，未指定语义。
 * SHT_DYNSYM: 表示动态链接符号表。
 * SHT_INIT_ARRAY: 表示指向初始化函数的指针数组。
 * SHT_FINI_ARRAY: 表示指向终止函数的指针数组。
 * SHT_PREINIT_ARRAY: 表示指向预初始化函数的指针数组。
 * SHT_GROUP: 表示段组。
 * SHT_SYMTAB_SHNDX: 表示扩展符号表的节索引。
 * SHT_NUM: 表示段类型的数量。
 * SHT_LOOS: 保留用于操作系统特定语义。
 * SHT_GNU_ATTRIBUTES: GNU特有的段属性。
 * SHT_GNU_HASH: GNU特有的符号散列表。
 * SHT_GNU_LIBLIST: GNU特有的预链接库列表。
 * SHT_CHECKSUM: 表示校验和段。
 * SHT_LOSUNW: Sun 特有的低保留段类型。
 * SHT_SUNW_COMDAT: Sun 特有的 COMDAT 段。
 * SHT_SUNW_syminfo: Sun 特有的符号信息段。
 * SHT_GNU_verdef: GNU特有的符号版本定义段。
 * SHT_GNU_verneed: GNU特有的符号版本需求段。
 * SHT_GNU_versym: GNU特有的符号版本表。
 */

int ft_process_build_elf_sym_table(struct elf *e)
{
	unsigned int i, j, k;
	char *SymStrTable;
#ifdef __x86_64__
	Elf64_Ehdr *ehdr;
	Elf64_Shdr *shdr;
	Elf64_Sym  *symtab;
#define ELF_ST_TYPE ELF64_ST_TYPE
#define Elf_Sym Elf64_Sym
#else
	Elf32_Ehdr *ehdr;
	Elf32_Shdr *shdr;
	Elf32_Sym  *symtab;
#define ELF_ST_TYPE ELF32_ST_TYPE
#define Elf_Sym Elf32_Sym
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
			symtab = (Elf_Sym *)&e->map[shdr[i].sh_offset];

			for (j = 0; j < shdr[i].sh_size / sizeof(Elf_Sym); j++, symtab++) {

				st_type = ELF_ST_TYPE(symtab->st_info);
				if (st_type != STT_FUNC)
					continue;

				switch(shdr[i].sh_type) {
					case SHT_SYMTAB:
						e->lsyms[e->lsc].name = xstrdup(&SymStrTable[symtab->st_name]);
						e->lsyms[e->lsc].type = shdr[i].sh_type;
						if (ehdr->e_type == ET_EXEC)
							e->lsyms[e->lsc].value = symtab->st_value;
						else
							e->lsyms[e->lsc].value = e->base_address + symtab->st_value;
						e->lsc++;
						break;
					case SHT_DYNSYM:
						e->dsyms[e->dsc].name = xstrdup(&SymStrTable[symtab->st_name]);
						e->dsyms[e->dsc].type = shdr[i].sh_type;
						if (ehdr->e_type == ET_EXEC)
							e->dsyms[e->dsc].value = symtab->st_value;
						else
							e->dsyms[e->dsc].value = e->base_address + symtab->st_value;
						e->dsc++;
						break;
				}
			}
		}
	}

	/*
	 * .plt 节区表项
	 * 在 64 位 ELF 文件中，.plt 节区用于动态链接函数调用。每个 .plt 表项的大小取决于特定架构和 ABI。以 x86_64 为例，每个表项通常是 16 字节对齐的，这是因为表项通常包含以下指令：
	 *
	 * pushq   GOT+8(%rip)
	 * jmpq    *GOT(%rip)
	 * <code to fixup GOT>
	 *
	 * .plt 节区的大小 (sh_size)
	 * sh_size 表示节区的总大小。由于 .plt 表项通常是 16 字节对齐的，如果 .plt 节区的表项个数是 n，那么总大小 sh_size 一般是 16 * n 字节。但是这不是严格的要求，而是由于 .plt 的设计通常导致这个结果。
	 */
	e->StringTable = (char *)&e->map[shdr[ehdr->e_shstrndx].sh_offset];
	for (i = 0; i < ehdr->e_shnum; i++) {
		if (!strcmp(&e->StringTable[shdr[i].sh_name], ".plt.sec")) {
			printf("shdr[i].sh_size %ld, %lx\n", shdr[i].sh_size, shdr[i].sh_addr);

			/*
			 * 19 0000000000401020 <.plt>:
			 * 20   401020:   ff 35 e2 2f 00 00       push   0x2fe2(%rip)        # 404008 <_GLOBAL_OFFSET_TABLE_+0x8>
			 * 21   401026:   f2 ff 25 e3 2f 00 00    bnd jmp *0x2fe3(%rip)        # 404010 <_GLOBAL_OFFSET_TABLE_+0x10>
			 * 22   40102d:   0f 1f 00                nopl   (%rax)
			 * 23   401030:   f3 0f 1e fa             endbr64                  <------- 第一个表项20个字节
			 * 24   401034:   68 00 00 00 00          push   $0x0
			 * 25   401039:   f2 e9 e1 ff ff ff       bnd jmp 401020 <_init+0x20>
			 * 26   40103f:   90                      nop
			 * 27   401040:   f3 0f 1e fa             endbr64                  <------- 剩余的表项，每个16字节
			 * 28   401044:   68 01 00 00 00          push   $0x1
			 * 29   401049:   f2 e9 d1 ff ff ff       bnd jmp 401020 <_init+0x20>
			 * 30   40104f:   90                      nop
			 * 31   401050:   f3 0f 1e fa             endbr64
			 *
			 *
			 *
			 * 79 Disassembly of section .plt.sec:
			 * 80 
			 * 81 0000000000401100 <puts@plt>:
			 * 82   401100:   f3 0f 1e fa             endbr64
			 * 83   401104:   f2 ff 25 0d 2f 00 00    bnd jmp *0x2f0d(%rip)        # 404018 <puts@GLIBC_2.2.5>
			 * 84   40110b:   0f 1f 44 00 00          nopl   0x0(%rax,%rax,1)
			 * 85 
			 * 86 0000000000401110 <sigaction@plt>:
			 * 87   401110:   f3 0f 1e fa             endbr64
			 * 88   401114:   f2 ff 25 05 2f 00 00    bnd jmp *0x2f05(%rip)        # 404020 <sigaction@GLIBC_2.2.5>
			 * 89   40111b:   0f 1f 44 00 00          nopl   0x0(%rax,%rax,1)
			 * 90 
			 * 91 0000000000401120 <getpid@plt>:
			 * 92   401120:   f3 0f 1e fa             endbr64
			 * 93   401124:   f2 ff 25 fd 2e 00 00    bnd jmp *0x2efd(%rip)        # 404028 <getpid@GLIBC_2.2.5>
			 * 94   40112b:   0f 1f 44 00 00          nopl   0x0(%rax,%rax,1)
			 * 95 
			 * 96 0000000000401130 <__stack_chk_fail@plt>:
			 * 97   401130:   f3 0f 1e fa             endbr64
			 * 98   401134:   f2 ff 25 f5 2e 00 00    bnd jmp *0x2ef5(%rip)        # 404030 <__stack_chk_fail@GLIBC_2.4>
			 * 99   40113b:   0f 1f 44 00 00          nopl   0x0(%rax,%rax,1)
			 */
			/* Skip first one. */
			for (k = 1, j = 0; j < shdr[i].sh_size; j += 16) {
				e->dsyms[k++].value = shdr[i].sh_addr + j; 
			}
			break;
		}
	} 

	return 0;
}

void ft_process_load_elf_section_range(struct elf *e)
{
#ifdef __x86_64__
	Elf64_Ehdr *ehdr;
	Elf64_Shdr *shdr;
#else
	Elf32_Ehdr *ehdr;
	Elf32_Shdr *shdr;
#endif

	char *StringTable;
	int i;

	e->shdr_count = 0;
	StringTable = e->StringTable;
	ehdr = e->ehdr;
	shdr = e->shdr;

	for (i = 0; i < ehdr->e_shnum; i++) {
		e->sh_range[i].sh_name = xstrdup(&StringTable[shdr[i].sh_name]);
		e->sh_range[i].sh_addr = shdr[i].sh_addr;
		e->sh_range[i].sh_size = shdr[i].sh_size;
		e->shdr_count++;
	}

	return;
}

int ft_process_is_special_area(char *name)
{
	int i = 0;
	char *specials[] = {
		"[heap]",
		"[stack]",
		"[vvar]",
		"[vdso]",
		"[vsyscall]",
	};

	for (i = 0; i < sizeof (specials) / sizeof (char *); i++)
	{
		if (memcmp(name, specials[i], strlen(name)+1) == 0)
			return 1;
	}

	return 0;
}

int ft_process_map_elf(struct elf *e)
{
	int fd;
	struct stat st;

	if ((fd = open(e->file, O_RDONLY)) < 0) {
		fprintf(stderr, "Unable to open %s: %s\n", e->file, strerror(errno));
		return -1;
	}

	if (fstat(fd, &st) < 0) {
		perror("fstat");
		return -1;
	}

	e->map = (uint8_t *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (e->map == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

#ifdef __x86_64__
	e->ehdr = (Elf64_Ehdr *)e->map;
	e->shdr = (Elf64_Shdr *)(e->map + e->ehdr->e_shoff);
	e->phdr = (Elf64_Phdr *)(e->map + e->ehdr->e_phoff);
#else
	e->ehdr = (Elf32_Ehdr *)e->map;
	e->shdr = (Elf32_Shdr *)(e->map + e->ehdr->e_shoff);
	e->phdr = (Elf32_Phdr *)(e->map + e->ehdr->e_phoff);
#endif

	e->StringTable = (char *)&e->map[e->shdr[e->ehdr->e_shstrndx].sh_offset];

	if (e->ehdr->e_shnum > 0 && e->ehdr->e_shstrndx != SHN_UNDEF)
		ft_process_load_elf_section_range(e);

	ft_process_build_elf_sym_table(e);

	return 0;
}

int ft_process_build_maps(struct process *p) 
{
	struct elf *new = NULL;
	struct elf *node = NULL;
	char line[BUFFER_SIZE];
	FILE *file = fopen(p->maps, "r");

	if (!file) {
		perror("Failed to open process map\n");
		return -1;
	}

	while (fgets(line, sizeof(line), file)) {
		unsigned long start, end;
		char perms[5], file_path[BUFFER_SIZE];
		int matched = sscanf(line, "%lx-%lx %4s %*s %*s %*s %s", &start, &end, perms, file_path);

		//log("%lx-%lx %s %s\n", start, end, perms, file_path);
		if (matched == 4) {
			size_t hash = elf_hash_by_name(file_path);

			/* First match is user app */
			if (!p->hash)
				p->hash = hash;

			node = (struct elf *)hmap_first_with_hash(p->elfs, hash);
			if (node == NULL)
			{
				new = xzalloc(sizeof (struct elf));
				if (new == NULL)
				{
					printf("Cannot create elf for %s\n", file_path);
					return -1;
				}
				new->hash = hash;
				new->base_address = start;
				sprintf(new->file, "%s", file_path);
				if (!ft_process_is_special_area(new->file))
					ft_process_map_elf(new);

				hmap_insert(p->elfs, (struct hmap_node *)new, new->hash);
				node = new;
			}
			sprintf(node->mas[node->nma].perms, "%s", perms);
			if (strncmp(file_path, "[heap]", strlen("[heap]")) == 0)
				node->mas[node->nma].type = MA_TYPE_HEAP;
			else if (strncmp(file_path, "[stack]", strlen("[stack]")) == 0)
				node->mas[node->nma].type = MA_TYPE_STACK;
			else if (strstr(node->mas[node->nma].perms, "x"))
				node->mas[node->nma].type = MA_TYPE_CODE;
			else
				node->mas[node->nma].type = MA_TYPE_DATA;
			node->mas[node->nma].svaddr = start;
			node->mas[node->nma].evaddr = end;
			node->mas[node->nma].size = end - start;
			node->nma++;
		}
	}

	fclose(file);

	return 0;
}

char *ft_get_process_path(int pid)
{
	char tmp[64], buf[256];
	char path[256], *ret, *p;
	FILE *fd;
	int i;

	snprintf(tmp, 64, "/proc/%d/maps", pid);

	if ((fd = fopen(tmp, "r")) == NULL) {
		fprintf(stderr, "Unable to open %s: %s\n", tmp, strerror(errno));
		exit(-1);
	}

	if (fgets(buf, sizeof(buf), fd) == NULL)
		return NULL;
	p = strchr(buf, '/');
	if (!p)
		return NULL;
	for (i = 0; *p != '\n' && *p != '\0'; p++, i++)
		path[i] = *p;
	path[i] = '\0';
	ret = (char *)HeapAlloc(i + 1);
	strcpy(ret, path);
	if (strstr(ret, ".so")) {
		fprintf(stderr, "Process ID: %d appears to be a shared library; file must be an executable. (path: %s)\n",pid, ret);
		exit(-1);
	}

	return ret;
}

struct process *ft_process_init(pid_t pid)
{
	struct process *p = &GP;

	p->pid = pid;
	p->elfs = xzalloc(sizeof (struct hmap));
	if (p->elfs == NULL)
	{
		printf("Failed to create elf map\n");
		return NULL;
	}
	hmap_init(p->elfs);

	sprintf(p->maps, "/proc/%d/maps", p->pid);
	GP.path = ft_get_process_path(p->pid);

	ft_process_build_maps(p);
	callstack_init(p);

	if (ptrace(PTRACE_ATTACH, p->pid, NULL, NULL) == -1) {
		perror("PTRACE_ATTACH");
		exit(-1);
	}

	p->attached = 1;

	return p;
}

struct memory_area *ft_process_get_memory_area_by_address(struct process *p, size_t vaddr)
{
	int i = 0;
	struct elf *node = NULL;

	HMAP_FOR_EACH (node, node, p->elfs) {
		for (i = 0; i < node->nma; i++)
		{
			if (vaddr >= node->mas[i].svaddr && vaddr <= node->mas[i].evaddr)
				return &node->mas[i];
		}
	}

	return NULL;
}

struct syms *ft_process_lookup_sym_by_addr(struct process *p, size_t addr)
{
	int i = 0;
	struct elf *node = NULL;

	HMAP_FOR_EACH (node, node, p->elfs) {
		for (i = 0; i < node->lsc; i++)
		{
			if (node->lsyms[i].value == addr)
				return &node->lsyms[i];
		}

		for (i = 0; i < node->dsc; i++)
		{
			if (node->dsyms[i].value == addr)
				return &node->dsyms[i];
		}
	}

	return NULL;
}

int distance(unsigned long a, unsigned long b)
{
	return ((a > b) ? (a - b) : (b - a));
}

#ifdef __x86_64__
char *getstr(unsigned long addr, int pid)
{	
	int i, j, c;
	uint8_t buf[sizeof(long)];
	char *string = (char *)HeapAlloc(256);
	unsigned long vaddr;

	string[0] = '"';
	for (c = 1, i = 0; i < 256; i += sizeof(long)) {
		vaddr = addr + i;

		if (pid_read(pid, buf, (void *)vaddr, sizeof(long)) == -1) {
			fprintf(stderr, "pid_read() failed: %s <0x%lx>\n", strerror(errno), vaddr);
			exit(-1);
		}

		for (j = 0; j < sizeof(long); j++) {

			if (buf[j] == '\n') {
				string[c++] = '\\';
				string[c++] = 'n';
				continue;
			}
			if (buf[j] == '\t') {
				string[c++] = '\\';
				string[c++] = 't';
				continue;
			}

			if (buf[j] != '\0' && isascii(buf[j]))
				string[c++] = buf[j];
			else
				goto out;
		}
	}

out:
	string[c++] = '"';
	string[c] = '\0';

	return string;	
}

/*
 * 常见的MOV指令操作码
 * MOV r/m64, r64 (将64位寄存器的值移动到内存或另一个寄存器)
 *
 * 操作码：0x89 /r
 * 例子：MOV [rax], rbx -> 0x89 18
 * MOV r64, r/m64 (将内存或64位寄存器的值移动到64位寄存器)
 *
 * 操作码：0x8B /r
 * 例子：MOV rbx, [rax] -> 0x8B 18
 * MOV r/m32, r32 (将32位寄存器的值移动到内存或另一个寄存器)
 *
 * 操作码：0x89 /r
 * 例子：MOV [rax], ebx -> 0x89 18
 * MOV r32, r/m32 (将内存或32位寄存器的值移动到32位寄存器)
 *
 * 操作码：0x8B /r
 * 例子：MOV ebx, [rax] -> 0x8B 18
 * MOV r/m16, r16 (将16位寄存器的值移动到内存或另一个寄存器)
 *
 * 操作码：0x66 0x89 /r
 * 例子：MOV [rax], bx -> 0x66 89 18
 * MOV r16, r/m16 (将内存或16位寄存器的值移动到16位寄存器)
 *
 * 操作码：0x66 0x8B /r
 * 例子：MOV bx, [rax] -> 0x66 8B 18
 * MOV r/m8, r8 (将8位寄存器的值移动到内存或另一个寄存器)
 *
 * 操作码：0x88 /r
 * 例子：MOV [rax], bl -> 0x88 18
 * MOV r8, r/m8 (将内存或8位寄存器的值移动到8位寄存器)
 *
 * 操作码：0x8A /r
 * 例子：MOV bl, [rax] -> 0x8A 18
 *
 * 操作码0xB8到0xBF分别对应将立即数加载到以下寄存器：
 *
 * 0xB8: MOV rax, imm64
 * 0xB9: MOV rcx, imm64
 * 0xBA: MOV rdx, imm64
 * 0xBB: MOV rbx, imm64
 * 0xBC: MOV rsp, imm64
 * 0xBD: MOV rbp, imm64
 * 0xBE: MOV rsi, imm64
 * 0xBF: MOV rdi, imm64
 *
 *
 * 使用场景
 * 0x41主要用于指示操作的寄存器是扩展寄存器集中的一个，例如r8到r15。具体来说，这意味着在ModR/M或SIB字节中使用扩展的寄存器。
 *
 * 示例：MOV r8, rax
 * 我们来看一个具体示例，使用0x41前缀指示操作涉及r8寄存器。
 *
 * 指令：MOV r8, rax
 * REX前缀：0x41
 * 操作码：0x89（MOV r/m64, r64）
 * ModR/M字节：C0（r8作为目标寄存器，rax作为源寄存器）
 * 最终编码为：
 *
 * 41 89 C0
 *
 * 详细分析
 * REX前缀：0x41
 * 操作码：0x89
 * MOV指令，将源寄存器的值移动到目标寄存器。
 * ModR/M字节：C0
 * C0编码表示目标寄存器是r8，源寄存器是rax。
 */

char *getargs(struct process *p, struct user_regs_struct *reg)
{
	unsigned char buf[12];
	int i, c, in_ptr_range = 0, j;
	char *args[256], *ptr;
	char tmp[512], *s;
	long val;
	char *string = (char *)HeapAlloc(MAXSTR);
	unsigned int maxstr = MAXSTR;
	unsigned int b;
	struct memory_area *ma = NULL;


	/* x86_64 supported only at this point--
	 * We are essentially parsing this
	 * calling convention here:
	 mov    %rsp,%rbp
	 mov    $0x6,%r9d
	 mov    $0x5,%r8d
	 mov    $0x4,%ecx
	 mov    $0x3,%edx
	 mov    $0x2,%esi
	 mov    $0x1,%edi
	 callq  400144 <func>
	 */

	for (c = 0, in_ptr_range = 0, i = 0; i < 35; i += 5) {

		val = reg->rip - i;
		if (pid_read(p->pid, buf, (void *)val, 8) == -1) {
			fprintf(stderr, "pid_read() failed [%d]: %s <0x%llx>\n", p->pid, strerror(errno), reg->rip);
			exit(-1);
		}

		printf("[%02x %02x %02x %02x %02x %02x] %llx\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], reg->rip);
		in_ptr_range = 0;
		if (buf[0] == 0x48 && buf[1] == 0x89 && buf[2] == 0xe5) // mov %rsp, %rbp
			break;
		switch((unsigned char)buf[0]) {
			case 0xbf:
				ma = ft_process_get_memory_area_by_address(p, reg->rdi);
				if (ma) {
					in_ptr_range++;
					switch(ma->type) {
						case MA_TYPE_CODE:
							s = getstr((unsigned long)reg->rdi, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}
							sprintf(tmp, "(text_ptr *)0x%llx", reg->rdi);
							break;
						case MA_TYPE_DATA:
							s = getstr((unsigned long)reg->rdi, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}
							sprintf(tmp, "(data_ptr *)0x%llx", reg->rdi);
							break;
						case MA_TYPE_HEAP:
							s = getstr((unsigned long)reg->rdi, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}

							sprintf(tmp, "(heap_ptr *)0x%llx", reg->rdi);
							break;
						case MA_TYPE_STACK:
							s = getstr((unsigned long)reg->rdi, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}
							sprintf(tmp, "(stack_ptr *)0x%llx", reg->rdi);
							break;
					}
				}
				if (!in_ptr_range) {
					sprintf(tmp, "0x%llx",reg->rdi);
				}	
				if (!s)
					args[c++] = xstrdup(tmp);
				break;
				sprintf(tmp, "0x%llx", reg->rdi);
				args[c++] = xstrdup(tmp);
				break;
			case 0xbe:
				ma = ft_process_get_memory_area_by_address(p, reg->rsi);
				if (ma) {
					in_ptr_range++;
					switch(ma->type) {
						case MA_TYPE_CODE:
							s = getstr((unsigned long)reg->rsi, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}

							sprintf(tmp, "(text_ptr *)0x%llx", reg->rsi);
							break;
						case MA_TYPE_DATA:
							s = getstr((unsigned long)reg->rsi, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}

							sprintf(tmp, "(data_ptr *)0x%llx", reg->rsi);
							break;
						case MA_TYPE_HEAP:
							s = getstr((unsigned long)reg->rsi, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}

							sprintf(tmp, "(heap_ptr *)0x%llx", reg->rsi);
							break;
						case MA_TYPE_STACK:
							s = getstr((unsigned long)reg->rsi, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}

							sprintf(tmp, "(stack_ptr *)0x%llx", reg->rsi);
							break;
					}
				}
				if (!in_ptr_range) {
					sprintf(tmp, "0x%llx", reg->rsi);
				}
				if (!s)
					args[c++] = xstrdup(tmp);
				break;

				sprintf(tmp, "0x%llx", reg->rsi);
				args[c++] = xstrdup(tmp);
				break;
			case 0xba:
				ma = ft_process_get_memory_area_by_address(p, reg->rdx);
				if (ma) {
					in_ptr_range++;
					switch(ma->type) {
						case MA_TYPE_CODE:
							s = getstr((unsigned long)reg->rdx, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}

							sprintf(tmp, "(text_ptr *)0x%llx", reg->rdx);
							break;
						case MA_TYPE_DATA:
							s = getstr((unsigned long)reg->rdx, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}
							sprintf(tmp, "(data_ptr *)0x%llx", reg->rdx);
							break;
						case MA_TYPE_HEAP:
							s = getstr((unsigned long)reg->rdx, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}
							sprintf(tmp, "(heap_ptr *)0x%llx", reg->rdx);
							break;
						case MA_TYPE_STACK:
							s = getstr((unsigned long)reg->rdx, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}
							sprintf(tmp, "(stack_ptr *)0x%llx", reg->rdx);
							break;
					}
				}
				if (!in_ptr_range) {
					sprintf(tmp, "0x%llx", reg->rdx);
				}
				if (!s)
					args[c++] = xstrdup(tmp);
				break;

				sprintf(tmp, "0x%llx", reg->rdx);
				args[c++] = xstrdup(tmp);
				break;
			case 0xb9:
				ma = ft_process_get_memory_area_by_address(p, reg->rcx);
				if (ma) {
					in_ptr_range++;
					switch(ma->type) {
						case MA_TYPE_CODE:
							s = getstr((unsigned long)reg->rcx, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}
							sprintf(tmp, "(text_ptr *)0x%llx", reg->rcx);
							break;
						case MA_TYPE_DATA:
							s = getstr((unsigned long)reg->rcx, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}
							sprintf(tmp, "(data_ptr *)0x%llx", reg->rcx);
							break;
						case MA_TYPE_HEAP:
							s = getstr((unsigned long)reg->rcx, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}
							sprintf(tmp, "(heap_ptr *)0x%llx", reg->rcx);
							break;
						case MA_TYPE_STACK:
							s = getstr((unsigned long)reg->rcx, p->pid);
							if (s) {
								snprintf(tmp, sizeof(tmp), "%s", s);
								args[c++] = xstrdup(tmp);
								break;
							}

							sprintf(tmp, "(stack_ptr *)0x%llx", reg->rcx);
							break;
					}
				}
				if (!in_ptr_range) {
					sprintf(tmp, "0x%llx", reg->rcx);
				}
				if (!s)
					args[c++] = xstrdup(tmp);
				break;

				sprintf(tmp, "0x%llx", reg->rcx);
				args[c++] = xstrdup(tmp);
				break;
			case 0x41:
				switch((unsigned char)buf[1]) {
					case 0xb8:
						ma = ft_process_get_memory_area_by_address(p, reg->r8);
						if (ma) {
							in_ptr_range++;
							switch(ma->type) {
								case MA_TYPE_CODE:
									s = getstr((unsigned long)reg->r8, p->pid);
									if (s) {
										snprintf(tmp, sizeof(tmp), "%s", s);
										args[c++] = xstrdup(tmp);
										break;
									}
									sprintf(tmp, "(text_ptr *)0x%llx", reg->r8);
									break;
								case MA_TYPE_DATA:
									s = getstr((unsigned long)reg->r8, p->pid);
									if (s) {
										snprintf(tmp, sizeof(tmp), "%s", s);
										args[c++] = xstrdup(tmp);
										break;
									}
									sprintf(tmp, "(data_ptr *)0x%llx", reg->r8);
									break;
								case MA_TYPE_HEAP:
									s = getstr((unsigned long)reg->r8, p->pid);
									if (s) {
										snprintf(tmp, sizeof(tmp), "%s", s);
										args[c++] = xstrdup(tmp);
										break;
									}
									sprintf(tmp, "(heap_ptr *)0x%llx", reg->r8);
									break;
								case MA_TYPE_STACK:
									s = getstr((unsigned long)reg->r8, p->pid);
									if (s) {
										snprintf(tmp, sizeof(tmp), "%s", s);
										args[c++] = xstrdup(tmp);
										break;
									}
									sprintf(tmp, "(stack_ptr *)0x%llx", reg->r8);
									break;
							}
						}
						if (!in_ptr_range) {
							sprintf(tmp, "0x%llx", reg->r8);
						}
						if (!s)
							args[c++] = xstrdup(tmp);
						break;

						sprintf(tmp, "0x%llx", reg->r8);
						args[c++] = xstrdup(tmp);
						break;
					case 0xb9:
						ma = ft_process_get_memory_area_by_address(p, reg->r9);
						if (ma) {
							in_ptr_range++;
							switch(ma->type) {
								case MA_TYPE_CODE:
									s = getstr((unsigned long)reg->r9, p->pid);
									if (s) {
										snprintf(tmp, sizeof(tmp), "%s", s);
										args[c++] = xstrdup(tmp);
										break;
									}
									sprintf(tmp, "(text_ptr *)0x%llx", reg->r9);
									break;
								case MA_TYPE_DATA:
									s = getstr((unsigned long)reg->r9, p->pid);
									if (s) {
										snprintf(tmp, sizeof(tmp), "%s", s);
										args[c++] = xstrdup(tmp);
										break;
									}
									sprintf(tmp, "(data_ptr *)0x%llx", reg->r9);
									break;
								case MA_TYPE_HEAP:
									s = getstr((unsigned long)reg->r9, p->pid);
									if (s) {
										snprintf(tmp, sizeof(tmp), "%s", s);
										args[c++] = xstrdup(tmp);
										break;
									}
									sprintf(tmp, "(heap_ptr *)0x%llx", reg->r9);
									break;
								case MA_TYPE_STACK:
									s = getstr((unsigned long)reg->r9, p->pid);
									if (s) {
										snprintf(tmp, sizeof(tmp), "%s", s);
										args[c++] = xstrdup(tmp);
										break;
									}
									sprintf(tmp, "(stack_ptr *)0x%llx", reg->r9);
									break;
							}       
						}
						if (!in_ptr_range) {
							sprintf(tmp, "0x%llx", reg->r9);
						}
						if (!s)
							args[c++] = xstrdup(tmp);
						break;       

						sprintf(tmp, "0x%llx", reg->r9);
						args[c++] = xstrdup(tmp);
						break;
				}
		}
	}

	/*
	 * XXX pre-allocation for strcpy/strcat, tested with super long function name
	 */
	if (c == 0)
		return NULL;

	for (b = 0, i = 0; i < c; i++) 
		b += strlen(args[i]) + 1; // len + ','
	if (b > maxstr + 2) { // maxstr + 2 braces
		string = realloc((char *)string, maxstr + (b - (maxstr + 2)) + 1);
		maxstr += (b - maxstr) + 3;
	}

	string[0] = '(';
	strcpy((char *)&string[1], args[0]);
	strcat(string, ",");

	for (i = 1; i < c; i++) {
		strcat(string, args[i]);
		strcat(string, ",");
	}

	if ((ptr = strrchr(string, ','))) 
		*ptr = '\0';
	strcat(string, ")");
	return string;
}
#endif

/*
 * 504   40166c:   e8 df fa ff ff          call   401150 <printf@plt>
 * 505   401671:   be 02 00 00 00          mov    $0x2,%esi
 * 506   401676:   bf 01 00 00 00          mov    $0x1,%edi
 * 507   40167b:   e8 61 fd ff ff          call   4013e1 <foo> 
 *
 * 在机器代码中，call 指令的操作数是相对于下一条指令的偏移量（即相对地址），而不是绝对地址。因此，当你看到类似 `call 401150 <printf@plt>` 和 `call 4013e1 <foo>` 这样的指令时，它们实际上是基于当前指令位置的偏移量。
 *
 * 让我们逐步解析这些指令以确定每个 call 指令目标地址的计算过程。
 *
 * ### 指令详细分析
 *
 * #### 指令 504 (0x40166c)
 * ```
 * 504   40166c:   e8 df fa ff ff          call   401150 <printf@plt>
 * ```
 *
 * ##### 解析：
 * 1. 当前指令的地址是 `0x40166c`。
 * 2. `e8 df fa ff ff` 是 call 指令，`e8` 是 call 指令的操作码。
 * 3. 紧跟着的 `df fa ff ff` 是 32 位的有符号偏移量（小端序表示）。
 *    - 这个偏移量是 `0xff ff fa df`（小端序，需要翻转字节顺序）。
 *       - 将其转换为有符号整数：`0xfffffadf`（十六进制），对应的十进制是 `-1345`。
 *
 *       ##### 目标地址计算：
 *       - 目标地址 = 当前指令地址 + 偏移量 + call 指令长度
 *       - 目标地址 = 0x40166c + (-1345) + 5
 *       - 目标地址 = 0x40166c - 1345 + 5
 *       - 目标地址 = 0x401150
 *
 * #### 指令 507 (0x40167b)
 * ```
 * 507   40167b:   e8 61 fd ff ff          call   4013e1 <foo>
 * ```
 *
 * ##### 解析：
 * 1. 当前指令的地址是 `0x40167b`。
 * 2. `e8 61 fd ff ff` 是 call 指令，`e8` 是 call 指令的操作码。
 * 3. 紧跟着的 `61 fd ff ff` 是 32 位的有符号偏移量（小端序表示）。
 *    - 这个偏移量是 `0xff ff fd 61`（小端序，需要翻转字节顺序）。
 *       - 将其转换为有符号整数：`0xfffffd61`（十六进制），对应的十进制是 `-671`。
 *
 *       ##### 目标地址计算：
 *       - 目标地址 = 当前指令地址 + 偏移量 + call 指令长度
 *       - 目标地址 = 0x40167b + (-671) + 5
 *       - 目标地址 = 0x40167b - 671 + 5
 *       - 目标地址 = 0x4013e1
 *
 * 因此，这两个 call 指令的目标地址是通过将偏移量加到当前指令的地址并再加上指令的长度（即5个字节）计算出来的。
 *
 *
 * # 条件分支指令（例如 `jmp`, `je`, `jne`, `jg`, `jl`, `jge`, `jle` 等）的跳转地址通常也是基于相对地址计算出来的。与 `call` 指令类似，这些条件分支指令的操作数也是相对于下一条指令的偏移量。
 *
 * ### 解析条件分支指令跳转地址
 *
 * 条件分支指令的格式通常是：
 * ```
 * opcode offset
 * ```
 * 其中 `opcode` 是条件分支指令的操作码，而 `offset` 是相对跳转的偏移量。偏移量是相对于条件分支指令的下一条指令计算的。
 *
 * #### 示例分析
 *
 * 假设我们有以下的汇编代码片段：
 * ```
 * 600   401680:  75 08                   jne    40168a
 * 601   401682:  be 02 00 00 00          mov    $0x2,%esi
 * 602   401687:  bf 01 00 00 00          mov    $0x1,%edi
 * 603   40168c:  e8 df fa ff ff          call   401150 <printf@plt>
 * ```
 *
 * ### 指令 600 (0x401680)
 * ```
 * 600   401680:  75 08                   jne    40168a
 * ```
 *
 * #### 解析：
 * 1. 当前指令的地址是 `0x401680`。
 * 2. `75` 是 `jne`（跳转如果不相等）的操作码。
 * 3. `08` 是 8 位的有符号偏移量。
 *
 * ##### 目标地址计算：
 * - `jne` 指令的目标地址 = 当前指令地址 + 偏移量 + 指令长度
 *   - 指令长度为 2 字节（1 字节的操作码 + 1 字节的偏移量）。
 *   - 目标地址 = 0x401680 + 0x08 + 2
 *   - 目标地址 = 0x401680 + 0x08 + 2 = 0x40168a
 *
 * ### 示例 2: 32 位偏移量
 *   条件分支指令也可以有 32 位的偏移量。假设我们有以下指令：
 *   ```
 *   700   401700:  0f 85 fa ff ff ff       jne    401700
 *   ```
 *
 * #### 解析：
 *   1. 当前指令的地址是 `0x401700`。
 *   2. `0f 85` 是 `jne`（跳转如果不相等）的操作码。
 *   3. `fa ff ff ff` 是 32 位的有符号偏移量（小端序）。
 *
 * ##### 目标地址计算：
 *   - `jne` 指令的目标地址 = 当前指令地址 + 偏移量 + 指令长度
 *   - 偏移量是 `0xfffffffA`（小端序，需要翻转字节顺序），对应十进制是 `-6`。
 *   - 指令长度为 6 字节（2 字节的操作码 + 4 字节的偏移量）。
 *   - 目标地址 = 0x401700 + (-6) + 6
 *   - 目标地址 = 0x401700
 *
 * ### 总结
 *   无论是 8 位还是 32 位偏移量，计算条件分支指令的目标地址的过程如下：
 *   1. 获取当前指令的地址。
 *   2. 读取偏移量（8 位或 32 位）。
 *   3. 计算目标地址：目标地址 = 当前指令地址 + 偏移量 + 指令长度（偏移量指的是从当前指令的下一条指令到目标地址的偏移量）。
 *
 * 这样，你就可以计算出条件分支指令的目标跳转地址。
 */

void ft_process_tracing(struct process *p)
{
	int status = 0;
	uint8_t buf[8];
	size_t current_ip = 0;
	__maybe_unused long esp, eax, ebx, edx, ecx, esi, edi, eip;
	struct memory_area *ma = NULL;
	struct user_regs_struct pt_reg;
	struct branch_instr *branch = NULL;
	char *argstr = NULL;
	calldata_t calldata;
	calldata_t *calldp;
	unsigned long vaddr;
	/* offset must be signed */
	int offset;
	struct syms *sym = NULL;
	char output[512];

	for (;;)
	{
		if (interrupted)
			break;

		ptrace (PTRACE_SINGLESTEP, p->pid, NULL, NULL);
		wait (&status);
		//	ptrace(PTRACE_GETREGS, h->pid, NULL, &pt_reg);

		if (WIFEXITED (status))
		{
			break;
		}

		ptrace (PTRACE_GETREGS, p->pid, NULL, &pt_reg);

#ifdef __x86_64__
		esp = pt_reg.rsp;
		eip = pt_reg.rip;
		eax = pt_reg.rax;
		ebx = pt_reg.rbx;
		ecx = pt_reg.rcx;
		edx = pt_reg.rdx;
		esi = pt_reg.rsi;
		edi = pt_reg.rdi;
#else
		esp = pt_reg.esp;
		eip = pt_reg.eip;
		eax = pt_reg.eax;
		ebx = pt_reg.ebx;
		ecx = pt_reg.ecx;
		edx = pt_reg.edx;
		esi = pt_reg.esi;
		edi = pt_reg.edi;
#endif
		ma = ft_process_get_memory_area_by_address(p, eip);
		if (!ma)
		{
			printf("Invalid eip %#lx\n", eip);
			exit(-1);
		}

		/* Read current instruction. */
		if (pid_read(p->pid, buf, (void *)eip, 8) < 0) {
			fprintf(stderr, "pid_read() failed: %s <0x%lx>\n", strerror(errno), eip);
			exit(-1);
		}

#if 0
		/* Branch instruction check. */
		if ((branch = search_branch_instr(buf[0])) != NULL) 
		{
			ptrace(PTRACE_SINGLESTEP, p->pid, NULL, NULL);
			wait(&status);

			ptrace(PTRACE_GETREGS, p->pid, NULL, &pt_reg);

			/* After jump */
#ifdef __x86_64__
			current_ip = pt_reg.rip;
#else
			current_ip = pt_reg.eip;
#endif
			ma = ft_process_get_memory_area_by_address(p, current_ip);
			if (!ma)
			{
				printf("Invalid eip %#lx\n", eip);
				exit(-1);
			}
		}
#endif

		/*
		 * Did we hit a breakpoint (Return address?)
		 * if so, then we check eax to get the return
		 * value, and pop the call data from the stack,
		 * which will remove the breakpoint as well.
		 */
		if (buf[0] == 0xcc) {
			calldp = callstack_peek(p);
			if (calldp != NULL) {
				if (calldp->retaddr == eip) {
					snprintf(output, sizeof(output), "%s(RETURN VALUE) %s%s = %lx\n", RED, WHITE, calldp->string, eax);

					/*
					 * Pop call stack and remove the
					 * breakpoint at its return address.
					 */
					fprintf(stdout, "%s", output);
					calldp = callstack_pop(p);
					free(calldp->string);
					free(calldp->symname);
				}
			}
		}

		/*
		 * As we catch each immediate call
		 * instruction, we use callstack_push()
		 * to push the call data onto our stack
		 * and set a breakpoint at the return
		 * address of the function call so that we
		 * can get the retrun value with the code above.
		 */

		if (buf[0] == 0xe8) {

			offset = buf[1] + (buf[2] << 8) + (buf[3] << 16) + (buf[4] << 24);
			vaddr = eip + offset + 5; 
#ifdef __x86_64__
			vaddr &= 0xffffffffffffffff;
#else
			vaddr &= 0xffffffff;
#endif

			sym = ft_process_lookup_sym_by_addr(p, vaddr);
			if (!sym)
			{
#if 0
				printf("Cannot find symble at %lx, %lx [%02x %02x %02x %02x %02x]\n", 
						vaddr, eip, buf[0], buf[1], buf[2], buf[3], buf[4]);
#endif
				continue;
				//exit(-1);
			}

#ifdef __x86_64__
			argstr = getargs(p, &pt_reg);
#endif
			if (argstr == NULL)
				printf("%scall@%s%s()\n", GREEN, WHITE, sym->name);
			else
				printf("%scall@%s%s%s\n", GREEN, WHITE, sym->name, argstr);

			calldata.symname = xstrdup(sym->name);
			calldata.vaddr = sym->value;
			calldata.retaddr = eip + 5;
			if (argstr == NULL) 
				calldata.string = xfmtstrdup("call@%s()", sym->name);
			else
				calldata.string = xfmtstrdup("call@%s(%s)", sym->name, argstr);

#if 0
			printf("Return address for %s: 0x%lx\n", calldata.symname, calldata.retaddr);
#endif

			callstack_push(p, &calldata);

			if (argstr) {
				free(argstr);
				argstr = NULL;
			}
		}
	}
}

int main(int argc, char **argv)
{
	pid_t pid = 0;
	struct process *p = NULL;

	if (argc > 2)
	{
		printf("Usage: %s [PROCESS_NAME]\n", argv[0]);
		return -1;
	}

	if (argc == 1)
		pid = getpid();
	else
		pid = get_pid_by_name(argv[1]);

	printf("[%s:%d]\n", argv[1], pid);
	p = ft_process_init(pid);

	ft_process_signal_init(p);

	ft_process_dump(p);

	ft_process_tracing(p);

	return 0;
}

__attribute__((destructor)) void __cleanup(void)
{
	int ret = 0;
	if (!GP.attached)
		return;

	callstack_flush(&GP);

	ptrace(PTRACE_CONT, GP.pid, NULL, NULL);
	ptrace(PTRACE_DETACH, GP.pid, NULL, NULL);
	GP.attached = 0;
}
