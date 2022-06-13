// #ifndef USERPROG_PROCESS_H
// #define USERPROG_PROCESS_H

// #include "threads/thread.h"

// tid_t process_create_initd (const char *file_name);
// tid_t process_fork (const char *name, struct intr_frame *if_);
// int process_exec (void *f_name);
// int process_wait (tid_t);
// void process_exit (void);
// void process_activate (struct thread *next);

// void argument_stack(char **parse, int count, void **esp);
// struct thread *get_child_process(int pid);
// void remove_child_process(struct thread *cp);

// /* pintos project3 */
// struct aux_struct {
// 	struct file *vmfile;
// 	off_t ofs;
// 	uint32_t read_bytes;
// 	uint32_t zero_bytes;
// 	bool is_loaded;
// 	bool writable;
// 	uint8_t* upage;
// };

// #endif /* userprog/process.h */

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

struct dummy {
    struct file *file;
    uint8_t *upage;
    off_t ofs;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
};

/* pintos project3 */
struct aux_struct {
	struct file *vmfile;
	off_t ofs;
	uint32_t read_bytes;
	uint32_t zero_bytes;
	bool is_loaded;
	bool writable;
	uint8_t* upage;
};

#endif /* userprog/process.h */