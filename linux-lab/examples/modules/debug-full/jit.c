#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/slab.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>

#include <asm/hardirq.h>

#define MODULE_NAME	"jit"
#define PROC_FILE_NR		8

/* use these as data pointers, to implement four files in one function */
enum jit_files {
	JIT_BUSY,
	JIT_SCHED,
	JIT_QUEUE,
	JIT_SCHEDTO
};

struct opt {
	int (*show)(struct seq_file *m, void *p);
	void *args;
};

/* This data structure used as "data" for the timer and tasklet functions */
struct jit_data {
	struct timer_list timer;
	struct tasklet_struct tlet;
	int hi; /* tasklet or tasklet_hi */
	wait_queue_head_t wait;
	unsigned long prevjiffies;
	unsigned char *buf;
	int loops;
};

#define JIT_ASYNC_LOOPS 5

int delay = HZ; /* the default delay, expressed in jiffies */
int tdelay = 10;

module_param(delay, int, 0);

static struct opt *opts[PROC_FILE_NR] = { NULL };

static int jit_currentime(struct seq_file *m, void *p)
{
	struct timespec64 tv1, tv2;
	unsigned long j1;
	u64 j2;

	pr_debug("%s() is invoked\n", __FUNCTION__);

	/* get them four */
	j1 = jiffies;
	j2 = get_jiffies_64();
	ktime_get_real_ts64(&tv1);
	ktime_get_coarse_real_ts64(&tv2);

	/* print */
	seq_printf(m ,"0x%08lx 0x%016Lx %10i.%06i\n"
		   "%41i.%09i\n",
		   j1, j2,
		   (int) tv1.tv_sec, (int) tv1.tv_nsec,
		   (int) tv2.tv_sec, (int) tv2.tv_nsec);
	return 0;
}

static int jit_fn(struct seq_file *m, void *p)
{
	unsigned long j0, j1; /* jiffies */
	wait_queue_head_t wait;
	extern int delay;

	pr_debug("%s() is invoked\n", __FUNCTION__);

	init_waitqueue_head(&wait);

	j0 = jiffies;
	j1 = j0 + delay;

	switch((long)(m->private)) {
	case JIT_BUSY:
		while (time_before(jiffies, j1))
			cpu_relax();
		break;
	case JIT_SCHED:
		while (time_before(jiffies, j1))
			schedule();
		break;
	case JIT_QUEUE:
		wait_event_interruptible_timeout(wait, 0, delay);
		break;
	case JIT_SCHEDTO:
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(delay);
		break;
	default:
		pr_debug("Known option\n");
	}

	seq_printf(m, "%9li %9li\n", j0, j1);

	return 0;
}

static void jit_timer_fn(struct timer_list *t)
{
	struct jit_data *data = from_timer(data, t, timer);
	unsigned long j = jiffies;

	pr_debug("%s() is invoked\n", __FUNCTION__);

	data->buf += sprintf(data->buf, "%9li  %3li     %i    %6i   %i   %s\n",
			     j, j - data->prevjiffies, in_interrupt() ? 1 : 0,
			     current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->timer.expires += tdelay;
		data->prevjiffies = j;
		add_timer(&data->timer);
	} else {
		wake_up_interruptible(&data->wait);
	}
}

/* the /proc function: allocate everything to allow concurrency */
static int jit_timer(struct seq_file *m, void *p)
{
	extern int tdelay;
	struct jit_data *data;
	char *buf = NULL, *buf2 = NULL;
	unsigned long j = jiffies;
	int retval = 0;

	pr_debug("%s() is invoked\n", __FUNCTION__);

	if (!(data = kmalloc(sizeof(struct jit_data), GFP_KERNEL)))
		return -ENOMEM;

	if (!(buf = kmalloc(PAGE_SIZE, GFP_KERNEL))) {
		retval = -ENOMEM;
		goto alloc_buf_error;
	}
	memset(buf, 0, PAGE_SIZE);
	buf2 = buf;

	timer_setup(&data->timer, jit_timer_fn, 0);
	init_waitqueue_head(&data->wait);

	buf2 += sprintf(buf2, "   time   delta  inirq    pid   cpu command\n");
	buf2 += sprintf(buf2, "%9li  %3li     %i    %6i   %i   %s\n",
			j, 0L, in_interrupt() ? 1 : 0,
			current->pid, smp_processor_id(), current->comm);

	/* fill the data for our timer function */
	data->prevjiffies = j;
	data->buf = buf2;
	data->loops = JIT_ASYNC_LOOPS;
	
	/* register the timer */
	data->timer.expires = j + tdelay; /* parameter */
	add_timer(&data->timer);

	wait_event_interruptible(data->wait, !data->loops);

	if (signal_pending(current)) {
		retval = -ERESTARTSYS;
		goto out;
	}

	seq_printf(m, "%s", buf);


out:
	kfree(buf);

alloc_buf_error:
	kfree(data);

	return retval;
}

static void jit_tasklet_fn(unsigned long arg)
{
	struct jit_data *data = (struct jit_data *)arg;
	unsigned long j = jiffies;

	pr_debug("%s() is invoked\n", __FUNCTION__);

	data->buf += sprintf(data->buf, "%9li  %3li     %i    %6i   %i   %s\n",
			     j, j - data->prevjiffies, in_interrupt() ? 1 : 0,
			     current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->prevjiffies = j;
		if (data->hi)
			tasklet_hi_schedule(&data->tlet);
		else
			tasklet_schedule(&data->tlet);
	} else {
		wake_up_interruptible(&data->wait);
	}
}

/* the /proc function: allocate everything to allow concurrency */
static int jit_tasklet(struct seq_file *m, void *p)
{
	struct jit_data *data;
	char *buf, *buf2;
	unsigned long j = jiffies;
	long hi = (long)(m->private);
	int retval = 0;

	pr_debug("%s() is invoked\n", __FUNCTION__);

	if (!(data = kmalloc(sizeof(struct jit_data), GFP_KERNEL)))
		return -ENOMEM;

	if (!(buf = kmalloc(PAGE_SIZE, GFP_KERNEL))) {
		retval = -ENOMEM;
		goto alloc_buf_error;
	}
	memset(buf, 0, PAGE_SIZE);
	buf2 = buf;

	init_waitqueue_head(&data->wait);

	/* write the first lines in the buffer */
	buf2 += sprintf(buf2, "   time   delta  inirq    pid   cpu command\n");
	buf2 += sprintf(buf2, "%9li  %3li     %i    %6i   %i   %s\n",
			j, 0L, in_interrupt() ? 1 : 0,
			current->pid, smp_processor_id(), current->comm);

	/* fill the data for our tasklet function */
	data->prevjiffies = j;
	data->buf = buf2;
	data->loops = JIT_ASYNC_LOOPS;

	/* register the tasklet */
	tasklet_init(&data->tlet, jit_tasklet_fn, (unsigned long)data);
	data->hi = hi;
	if (hi)
		tasklet_hi_schedule(&data->tlet);
	else
		tasklet_schedule(&data->tlet);

	/* wait for the buffer to fill */
	wait_event_interruptible(data->wait, !data->loops);

	if (signal_pending(current)) {
		goto out;
		retval = -ERESTARTSYS;
	}

	seq_printf(m, "%s", buf);

out:
	kfree(buf);

alloc_buf_error:
	kfree(data);

	return retval;
}

static
int proc_open(struct inode *inode, struct file *filp)
{
	struct opt *opt = pde_data(inode);
	return single_open(filp, opt->show, opt->args);
}

static
int proc_release(struct inode *inode, struct file *filp)
{
	return single_release(inode, filp);
}

static struct proc_ops proc_ops = {
	.proc_open    = proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = proc_release,
};


static inline
struct opt *new_opt(int (*show)(struct seq_file *m, void *p), void *args)
{
	struct opt *opt = kmalloc(sizeof(struct opt), GFP_KERNEL);
	opt->show = show;
	opt->args = args;

	return opt;
}

static
int __init m_init(void)
{
	struct opt *opt = NULL;
	int i = 0;

	printk(KERN_WARNING MODULE_NAME " is loaded\n");

	opt = opts[i++] = new_opt(jit_currentime, NULL);
	proc_create_data("currentime", 0, NULL, &proc_ops, opt);

	opt = opts[i++] = new_opt(jit_fn, (void*)JIT_BUSY);
	proc_create_data("jitbusy",   0, NULL, &proc_ops, opt);
	opt = opts[i++] = new_opt(jit_fn, (void*)JIT_SCHED);
	proc_create_data("jitsched",  0, NULL, &proc_ops, opt);
	opt = opts[i++] = new_opt(jit_fn, (void*)JIT_QUEUE);
	proc_create_data("jitqueue",  0, NULL, &proc_ops, opt);
	opt = opts[i++] = new_opt(jit_fn, (void*)JIT_SCHEDTO);
	proc_create_data("jitschedto",0, NULL, &proc_ops, opt);

	opt = opts[i++] = new_opt(jit_timer, NULL);
	proc_create_data("jitimer", 0, NULL, &proc_ops, opt);

	opt = opts[i++] = new_opt(jit_tasklet, NULL);
	proc_create_data("jitasklet", 0, NULL, &proc_ops, opt);

	opt = opts[i++] = new_opt(jit_tasklet, (void*)1);
	proc_create_data("jitasklethi", 0, NULL, &proc_ops, opt);

	return 0;
}

static
void __exit m_exit(void)
{
	printk(KERN_WARNING MODULE_NAME " unloaded\n");

	remove_proc_entry("currentime", NULL);
	remove_proc_entry("jitbusy", NULL);
	remove_proc_entry("jitsched", NULL);
	remove_proc_entry("jitqueue", NULL);
	remove_proc_entry("jitschedto", NULL);
	remove_proc_entry("jitimer", NULL);
	remove_proc_entry("jitasklet", NULL);
	remove_proc_entry("jitasklethi", NULL);

	for (int i = 0; i < PROC_FILE_NR; ++i)
		kfree(opts[i]);

#if 0
	remove_proc_entry("jitasklethi", NULL);
#endif
}

module_init(m_init);
module_exit(m_exit);

MODULE_AUTHOR("d0u9");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Delay methods in Linux kernel.");

