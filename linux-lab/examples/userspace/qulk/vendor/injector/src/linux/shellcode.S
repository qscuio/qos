#if defined(__x86_64__)
#define handle_offset 0
#define dlopen_addr_offset 8
#define dlerror_addr_offset 16
#define dlflags_offset 24
#define file_path_offset 28
#define page_size 4096
	//  这些伪指令定义了代码段（.text）并声明了全局符号injector_shellcode，同时隐藏了这个符号（防止外部可见），并指定它的类型为函数。
	.text
	.global	injector_shellcode
	.hidden	injector_shellcode
	.type	injector_shellcode, @function
	// void *injector_shellcode(injector_shellcode_arg_t *arg) {
	// 这是注入器shellcode的入口点，类似于C语言中的函数定义。
injector_shellcode:
	//   // prolog
	//   函数的前置操作，保存寄存器%rbx的值，并将第一个函数参数（指向injector_shellcode_arg_t结构体的指针）保存到%rbx中
	pushq	%rbx
	movq	%rdi, %rbx
	//   从结构体arg中加载dlflags到寄存器%esi。
	//   int dlflags = arg->dlflags;
	movl	dlflags_offset(%rbx), %esi
	//   计算出file_path的地址并存入寄存器%rdi。
	//   const char *file_path = arg->file_path;
	leaq	file_path_offset(%rbx), %rdi
	//   调用dlopen函数，参数从%rdi和%esi传递，结果存入%rax。
	//   void *handle = dlopen(file_path, dlflags);
	call	*dlopen_addr_offset(%rbx)
	//   arg->handle = handle;
	//   将dlopen的返回值（句柄）存储到结构体arg的handle字段。
	movq	%rax, handle_offset(%rbx)
	//   将file_path的第一个字符置为终止符\0
	//   arg->file_path[0] = '\0';
	movb	$0, file_path_offset(%rbx)
	//   检查dlopen返回值，如果不为空，跳转到.exit标签，结束函数。
	//   if (handle != NULL) return;
	test	%rax, %rax
	jnz	.exit
	//   检查dlerror函数的地址，如果为空，跳转到.exit标签，结束函数。
	//   if (arg->dlerror_addr == 0) return;
	cmpq	$0, dlerror_addr_offset(%rbx)
	je	.exit
	//   调用dlerror函数，获取错误信息指针到%rax。
	//   char *errmsg = dlerror();
	call	*dlerror_addr_offset(%rbx)
	//   检查错误信息指针是否为空，如果为空，跳转到.exit标签，结束函数。
	//   if (errmsg == NULL) return;
	test	%rax, %rax
	jz	.exit
	//   设置dest指向file_path，设置end指向结构体末尾（假设一页大小）。
	//   char *dest = arg->file_path
	leaq	file_path_offset(%rbx), %rdi
	//   char *end = (char*)arg + page_size;
	leaq	page_size(%rbx), %rcx
	//   循环复制错误信息字符串到file_path，直到遇到终止符或达到页末。
.loop:
	//   char c = *(errmsg++);
	movb	(%rax), %dl
	addq	$1, %rax
	//   *(dest++) = c;
	movb	%dl, (%rdi)
	addq	$1, %rdi
	//   if (c == 0) return;
	testb	%dl, %dl
	jz	.exit
	//   if (dest < end) goto loop;
	cmpq	%rdi, %rcx
	ja	.loop
.exit:
	//   函数结束，恢复寄存器%rbx的值并返回。计算函数大小并定义injector_shellcode的大小。
	//   // epilog
	popq	%rbx
	ret
	// } 
	.size	injector_shellcode, . - injector_shellcode

	.balign  4
	.global	injector_shellcode_size
	.hidden	injector_shellcode_size
	.type	injector_shellcode_size, @object
	.size	injector_shellcode_size, 4
injector_shellcode_size:
	// distance from injector_shellcode to current.
	.int	. - injector_shellcode
#endif
