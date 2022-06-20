#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);

void argument_stack(char **parse, int count, void **esp);
struct thread *get_child_process(int pid);
void remove_child_process(struct thread *cp);

#endif /* userprog/process.h */