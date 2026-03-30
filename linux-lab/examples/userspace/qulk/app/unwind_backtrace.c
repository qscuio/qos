#include <stdio.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>

void do_backtrace2()
{
    unw_cursor_t    cursor;
    unw_context_t   context;
 
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
    while (unw_step(&cursor) > 0) {
        unw_word_t  offset;
        unw_word_t  pc, eax, ebx, ecx, edx;
        char        fname[64];
 
        unw_get_reg(&cursor, UNW_REG_IP,  &pc);
        unw_get_reg(&cursor, UNW_X86_64_RAX, &eax);
        unw_get_reg(&cursor, UNW_X86_64_RDX, &edx);
        unw_get_reg(&cursor, UNW_X86_64_RCX, &ecx);
        unw_get_reg(&cursor, UNW_X86_64_RBX, &ebx);
 
        fname[0] = '\0';
        unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
        printf ("%lx : (%s+0x%lx) [%lx]\n", pc, fname, offset, pc);
        printf("\tEAX=0x%08lx EDX=0x%08lx ECX=0x%08lx EBX=0x%08lx\n",
                eax, edx, ecx, ebx);
    }
}
 
void do_backtrace(void)
{
    unw_cursor_t    cursor;
    unw_context_t   context;
 
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
 
    while (unw_step(&cursor) > 0) {
        unw_word_t  offset, pc;
        char        fname[64];
 
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
 
        fname[0] = '\0';
        (void) unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
 
        printf ("%lx : (%s+0x%lx) [%lx]\n", pc, fname, offset, pc);
    }

    return;
}

void backtrace2(void)
{
    do_backtrace();
    do_backtrace2();
}

void backtrace(void)
{
    backtrace2();
}
 
int main()
{
    backtrace();
    return 0;
}
