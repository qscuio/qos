#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/*
 * To Check Context switch by gdb/qemu we need the app run in user space and kernel space almost the same time.
 *
* (gdb) info thread
* Id   Target Id                    Frame 
* * 1    Thread 1.1 (CPU#0 [halted ]) default_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/process.c:743
* 2    Thread 1.2 (CPU#1 [halted ]) native_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:37
* 3    Thread 1.3 (CPU#2 [running]) 0x000055ec52f7b175 in ?? ()
* 4    Thread 1.4 (CPU#3 [halted ]) native_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:37
* (gdb) thread apply all bt
* 
* Thread 4 (Thread 1.4 (CPU#3 [halted ])):
* #0  native_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:37
* #1  arch_local_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:72
* #2  default_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/process.c:743
* #3  0xffffffff8218522c in default_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:117
* #4  0xffffffff811240c9 in cpuidle_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:191
* #5  do_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:332
* #6  0xffffffff81124329 in cpu_startup_entry (state=state@entry=CPUHP_AP_ONLINE_IDLE) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:430
* #7  0xffffffff8107e42c in start_secondary (unused=<optimized out>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/smpboot.c:313
* #8  0xffffffff8103c764 in secondary_startup_64 () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head_64.S:420
* #9  0x0000000000000000 in ?? ()
* 
* Thread 3 (Thread 1.3 (CPU#2 [running])):                    >>>>> User space 
* #0  0x000055ec52f7b175 in ?? ()
* #1  0x000000034e0ac510 in ?? ()
* #2  0x0000207700000000 in ?? ()
* #3  0x000055ec52f7b1b0 in ?? ()
* #4  0x00007fc1c687ed0a in ?? ()
* #5  0x00007ffc4e0ac518 in ?? ()
* #6  0x0000000100000000 in ?? ()
* #7  0x000055ec52f7b145 in ?? ()
* #8  0x00007fc1c687e7cf in ?? ()
* #9  0x0000000000000000 in ?? ()
* 
* Thread 2 (Thread 1.2 (CPU#1 [halted ])):
* #0  native_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:37
* #1  arch_local_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:72
* #2  default_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/process.c:743
* #3  0xffffffff8218522c in default_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:117
* #4  0xffffffff811240c9 in cpuidle_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:191
* #5  do_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:332
* #6  0xffffffff81124329 in cpu_startup_entry (state=state@entry=CPUHP_AP_ONLINE_IDLE) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:430
* #7  0xffffffff8107e42c in start_secondary (unused=<optimized out>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/smpboot.c:313
* #8  0xffffffff8103c764 in secondary_startup_64 () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head_64.S:420
* #9  0x0000000000000000 in ?? ()
* 
* Thread 1 (Thread 1.1 (CPU#0 [halted ])):
* #0  default_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/process.c:743
* #1  0xffffffff8218522c in default_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:117
* #2  0xffffffff811240c9 in cpuidle_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:191
* #3  do_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:332
* #4  0xffffffff81124329 in cpu_startup_entry (state=state@entry=CPUHP_ONLINE) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:430
* #5  0xffffffff82185c17 in rest_init () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/init/main.c:739
* #6  0xffffffff83b29274 in start_kernel () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/init/main.c:1081
* #7  0xffffffff83b351e8 in x86_64_start_reservations (real_mode_data=real_mode_data@entry=0x14750 <entry_stack_storage+1872> <error: Cannot access memory at address 0x14750>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head64.c:507
* #8  0xffffffff83b35326 in x86_64_start_kernel (real_mode_data=0x14750 <entry_stack_storage+1872> <error: Cannot access memory at address 0x14750>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head64.c:488
* #9  0xffffffff8103c764 in secondary_startup_64 () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head_64.S:420
* #10 0x0000000000000000 in ?? ()
* (gdb)
* 
* (gdb) info thread
*   Id   Target Id                    Frame 
* * 1    Thread 1.1 (CPU#0 [halted ]) default_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/process.c:743
*   2    Thread 1.2 (CPU#1 [halted ]) native_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:37
*   3    Thread 1.3 (CPU#2 [running]) preempt_count () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/preempt.h:26
*   4    Thread 1.4 (CPU#3 [halted ]) native_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:37
* (gdb) thread apply all bt
* 
* Thread 4 (Thread 1.4 (CPU#3 [halted ])):
* #0  native_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:37
* #1  arch_local_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:72
* #2  default_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/process.c:743
* #3  0xffffffff8218522c in default_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:117
* #4  0xffffffff811240c9 in cpuidle_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:191
* #5  do_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:332
* #6  0xffffffff81124329 in cpu_startup_entry (state=state@entry=CPUHP_AP_ONLINE_IDLE) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:430
* #7  0xffffffff8107e42c in start_secondary (unused=<optimized out>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/smpboot.c:313
* #8  0xffffffff8103c764 in secondary_startup_64 () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head_64.S:420
* #9  0x0000000000000000 in ?? ()
* 
* Thread 3 (Thread 1.3 (CPU#2 [running])):      ### ----------------> Kernel space
* #0  preempt_count () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/preempt.h:26
* #1  check_kcov_mode (needed_mode=KCOV_MODE_TRACE_PC, t=0xffff888103035880) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/kcov.c:173
* #2  __sanitizer_cov_trace_pc () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/kcov.c:207
* #3  0xffffffff8138039c in filp_flush (filp=filp@entry=0xffff88810338de00, id=0xffff888100067400) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/fs/open.c:1518
* #4  0xffffffff81384f40 in __do_sys_close (fd=<optimized out>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/current.h:49
* #5  __se_sys_close (fd=<optimized out>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/fs/open.c:1541
* #6  __x64_sys_close (regs=0xffffc900025b3f58) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/fs/open.c:1541
* #7  0xffffffff8217dff8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900025b3f58) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/entry/common.c:52
* #8  do_syscall_64 (regs=0xffffc900025b3f58, nr=<optimized out>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/entry/common.c:83
* #9  0xffffffff82200130 in entry_SYSCALL_64 () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/entry/entry_64.S:121
* #10 0x0000000000000000 in ?? ()
* 
* Thread 2 (Thread 1.2 (CPU#1 [halted ])):
* #0  native_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:37
* #1  arch_local_irq_disable () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/include/asm/irqflags.h:72
* #2  default_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/process.c:743
* #3  0xffffffff8218522c in default_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:117
* #4  0xffffffff811240c9 in cpuidle_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:191
* #5  do_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:332
* #6  0xffffffff81124329 in cpu_startup_entry (state=state@entry=CPUHP_AP_ONLINE_IDLE) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:430
* #7  0xffffffff8107e42c in start_secondary (unused=<optimized out>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/smpboot.c:313
* #8  0xffffffff8103c764 in secondary_startup_64 () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head_64.S:420
* #9  0x0000000000000000 in ?? ()
* 
* Thread 1 (Thread 1.1 (CPU#0 [halted ])):
* #0  default_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/process.c:743
* #1  0xffffffff8218522c in default_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:117
* #2  0xffffffff811240c9 in cpuidle_idle_call () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:191
* #3  do_idle () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:332
* #4  0xffffffff81124329 in cpu_startup_entry (state=state@entry=CPUHP_ONLINE) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/kernel/sched/idle.c:430
* #5  0xffffffff82185c17 in rest_init () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/init/main.c:739
* #6  0xffffffff83b29274 in start_kernel () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/init/main.c:1081
* #7  0xffffffff83b351e8 in x86_64_start_reservations (real_mode_data=real_mode_data@entry=0x14750 <entry_stack_storage+1872> <error: Cannot access memory at address 0x14750>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head64.c:507
* #8  0xffffffff83b35326 in x86_64_start_kernel (real_mode_data=0x14750 <entry_stack_storage+1872> <error: Cannot access memory at address 0x14750>) at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head64.c:488
* #9  0xffffffff8103c764 in secondary_startup_64 () at /home/qwert/work/qwert/Linux/kernel/ulk/kernel/linux-6.9.6/arch/x86/kernel/head_64.S:420
* #10 0x0000000000000000 in ?? ()
* (gdb)
*/
 
int main()
{
    int fd = 0;
    int i = 0;
    int j = 0;

    for (;;)
    {
	for (i = 0; i < 10000; i++)
	{
	    j *= i;
	    j += j;
	}

	fd = open("/dev/console", O_RDONLY);
	close(fd);
    }
}
