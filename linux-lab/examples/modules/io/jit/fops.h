#ifndef _FOPS_H
#define _FOPS_H

int jit_currentime(struct seq_file *m, void *p);
void jit_tasklet_fn(unsigned long arg);
void jit_timer_fn(struct timer_list *t);
extern int jit_currentime(struct seq_file *m, void *p);
extern int jit_fn(struct seq_file *m, void *p);
extern int jit_timer(struct seq_file *m, void *p);
extern int jit_tasklet(struct seq_file *m, void *p);

#endif
