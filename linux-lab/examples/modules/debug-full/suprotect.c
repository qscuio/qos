#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/mmu_context.h>
#include <linux/workqueue.h>
#include <linux/mm_types.h>
#include <linux/sched/mm.h>
#include <linux/sched/task.h>
#include <linux/kthread.h>
#include <linux/kprobes.h>

#include <linux/ioctl.h>
#include <linux/types.h>

/*
   Let's run python with an infinite loop:

   ron@ron-ubuntu:~/suprotect$ python -c "while True: pass" &
   [1] 4504
   First, check its memory mapping:

   ron@ron-ubuntu:~/suprotect$ sudo cat /proc/4504/maps
   56143b00d000-56143b315000 r-xp 00000000 08:01 1579941                    /usr/bin/python2.7
   56143b515000-56143b517000 r--p 00308000 08:01 1579941                    /usr/bin/python2.7
   56143b517000-56143b58d000 rw-p 0030a000 08:01 1579941                    /usr/bin/python2.7
   ...
   Remove the execute access for the entire executable region (prot=1 is read only):

   ron@ron-ubuntu:~/suprotect$ sudo ./suprotect-cli 4504 0x56143b00d000 0x308000 1
   Immediately, the python process is killed due to segmentation fault:

   [1]+  Segmentation fault      (core dumped) python -c "while True: pass"
 */

#define SUPROTECT_DEV_NAME          "suprotect"
#define SUPROTECT_IOCTL_MPROTECT    _IOW('7', 1, struct suprotect_request)

struct suprotect_request {
    /* pid of the target task */
    pid_t pid;

    /* parameters to mprotect */
    void *addr;
    size_t len;
    int prot;
};


int (*do_mprotect_pkey)(unsigned long start, size_t len,
                        unsigned long prot, int pkey) = NULL;

struct suprotect_work {
    struct work_struct work;

    struct mm_struct *mm;
    unsigned long start;
    size_t len;
    unsigned long prot;
    int ret_value;
};

static void suprotect_work(struct work_struct *work)
{
    struct suprotect_work *suprotect_work = container_of(work,
                                                         struct suprotect_work,
                                                         work);

    kthread_use_mm(suprotect_work->mm);
    suprotect_work->ret_value = do_mprotect_pkey(suprotect_work->start,
                                                 suprotect_work->len,
                                                 suprotect_work->prot, -1);
    kthread_unuse_mm(suprotect_work->mm);
}

static long suprotect_ioctl(struct file *file, unsigned int cmd,
                            unsigned long arg)
{
    long ret;
    struct suprotect_request params;
    struct pid *pid_struct = NULL;
    struct task_struct *task = NULL;
    struct mm_struct *mm = NULL;
    struct suprotect_work work;

    if (cmd != SUPROTECT_IOCTL_MPROTECT)
        return -ENOTTY;

    if (copy_from_user(&params, (struct suprotect_request *) arg,
                       sizeof(params)))
        return -EFAULT;

    /* Find the task by the pid */
    pid_struct = find_get_pid(params.pid);
    if (!pid_struct)
        return -ESRCH;

    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task) {
        ret = -ESRCH;
        goto out;
    }

    /* Get the mm of the task */
    mm = get_task_mm(task);
    if (!mm) {
        ret = -ESRCH;
        goto out;
    }

    /* Initialize work */
    INIT_WORK(&work.work, suprotect_work);

    work.mm = mm;
    work.start = (unsigned long) params.addr;
    work.len = params.len;
    work.prot = params.prot;

    /* Queue the work */
    (void) schedule_work(&work.work);

    /* Wait for completion of the work */
    flush_work(&work.work);
    ret = work.ret_value;

out:
    if (mm) mmput(mm);
    if (task) put_task_struct(task);
    if (pid_struct) put_pid(pid_struct);

    return ret;
}

static const struct file_operations suprotect_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = suprotect_ioctl,
};

static struct miscdevice suprotect_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = SUPROTECT_DEV_NAME,
    .fops = &suprotect_fops,
};

static unsigned long lookup_name(const char *name)
{
    struct kprobe kp = {
        .symbol_name = name
    };
    unsigned long retval;

    if (register_kprobe(&kp) < 0) return 0;
    retval = (unsigned long) kp.addr;
    unregister_kprobe(&kp);
    return retval;
}

static int suprotect_init(void)
{
    int err;

    do_mprotect_pkey = (void *) lookup_name("do_mprotect_pkey");
    if (!do_mprotect_pkey) {
        printk(KERN_ERR "Could not find do_mprotect_pkey.\n");
        return -ENOSYS;
    }

    err = misc_register(&suprotect_dev);
    if (err < 0) {
        printk(KERN_ERR "Could not register misc device, error: %d.\n", err);
        return err;
    }

    return 0;
}

static void suprotect_cleanup(void)
{
    misc_deregister(&suprotect_dev);
}

MODULE_LICENSE("GPL");
module_init(suprotect_init);
module_exit(suprotect_cleanup);

