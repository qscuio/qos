#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 3)
#include <linux/sched/task.h>
#endif
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kanatip Chitavisutthivong");
MODULE_DESCRIPTION("Involves iterating over all tasks in the system using DFS tree");
MODULE_VERSION("0.1");

static void dfs(struct task_struct *task)
{
	struct task_struct *next_task;
	struct list_head *list;
	list_for_each(list, &task->children) {
		next_task = list_entry(list, struct task_struct, sibling);
		printk(KERN_INFO "[pid: %d] pname: %s [state: %d]\n",next_task->pid, next_task->comm, next_task->__state);
		dfs(next_task);
	}	
}
static int dfs_tree_tasks_init(void)
{
	printk(KERN_INFO "Loading module\n");
	dfs(&init_task);
	return 0;
}	

static void dfs_tree_tasks_exit(void)
{
	printk(KERN_INFO "Removing module\n");
}

module_init(dfs_tree_tasks_init);
module_exit(dfs_tree_tasks_exit);


