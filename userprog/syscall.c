#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "include/threads/init.h"
#include "include/filesys/filesys.h"
#include "include/userprog/process.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	check_address(f->rsp);
	printf("check address done!\n");
	switch (f->R.rax) {
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			exit(0);
			break;
		case SYS_EXEC:
			break;
		case SYS_WAIT:
			break;
		case SYS_CREATE:
			// create(f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			// remove(f->R.rdi);
			break;
		case SYS_OPEN:
			break;
		case SYS_FILESIZE:
			break;
		case SYS_READ:
			break;
		case SYS_WRITE:
			break;
		case SYS_SEEK:
			break;
		case SYS_TELL:
			break;
		case SYS_CLOSE:
			break;
		default:
			break;
	}
	// printf ("system call!\n");
	// thread_exit ();
}

void check_address(void *addr) {
	struct thread *cur = thread_current();
	if (is_kernel_vaddr(addr) || pml4_get_page(cur->pml4, addr) == NULL) {
		exit(-1);
	}
}

void halt() {
	power_off();
}

void exit(int status) {
	// struct thread *cur = thread_current();
	// printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

// bool create(const char *file, unsigned initial_size) {
// 	return filesys_create(file, initial_size);
// }

// bool remove(const char *file) {
// 	return filesys_remove(file);
// }

tid_t exec(const char *cmd_line) {
	check_address(cmd_line);
	int file_size = strlen(cmd_line) + 1;
	char *fn_copy

}
