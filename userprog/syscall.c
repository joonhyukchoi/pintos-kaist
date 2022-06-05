#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

// * USERPROG 추가
#include "threads/palloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

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

  lock_init(&filesys_lock);

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
  
  switch (f->R.rax) {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      exit(f->R.rdi);
      break;
    case SYS_FORK:
      memcpy(&thread_current()->fork_tf, f, sizeof(struct intr_frame));
      f->R.rax = fork(f->R.rdi);
      break;
    case SYS_EXEC:
      if(exec(f->R.rdi) == -1) 
        exit(-1);
      break;
    case SYS_WAIT:
      f->R.rax = wait(f->R.rdi);
      break; 
    case SYS_CREATE:
      f->R.rax = create(f->R.rdi, f->R.rsi);
      break;
    case SYS_REMOVE:
      f->R.rax = remove(f->R.rdi);
      break;
    case SYS_OPEN:
      f->R.rax = open(f->R.rdi);
      break;
    case SYS_FILESIZE:
      f->R.rax = filesize(f->R.rdi);
      break;
    case SYS_READ:
      f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
      break;  
    case SYS_WRITE:      
      f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
      break;
    case SYS_SEEK:
      seek(f->R.rdi, f->R.rsi);
      break;
    case SYS_TELL:
      f->R.rax = tell(f->R.rdi);
      break;
    case SYS_CLOSE:
      close(f->R.rdi);
      break;
    default:
      exit(-1);
      break;
  }
}

void halt(void) {
  // * power_off()를 사용하여 pintos 종료
  power_off();
}

void exit(int status) {
  /*
   * 실행중인 스레드 구조체를 가져옴
   * 프로세스 종료 메시지 출력
   * 출력 양식: "프로세스 이름: exit(종료상태)"
   * thread 종료
   */ 
  struct thread *cur = thread_current();
  cur->exit_status = status;
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

tid_t fork (const char *thread_name) {
  check_address(thread_name);
  return process_fork(thread_name, &thread_current()->fork_tf);
}

int exec (const char *cmd_line) {
  check_address(cmd_line);

  int file_size = strlen(cmd_line) + 1;
  char *fn_copy = palloc_get_page(PAL_ZERO);
  if (fn_copy == NULL) {
    return -1;
  }
  strlcpy(fn_copy, cmd_line, file_size);

  if (process_exec(fn_copy) == -1) {
    return -1;
  }
}

int wait (tid_t pid) {
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size) {
  /* 
   * 파일 이름과 크기에 해당하는 파일 생성
   * 파일 생성 성공 시 true 반환, 실패 시 false 반환
   */
  check_address(file);
  return filesys_create(file, initial_size);
}

bool remove (const char *file) {
  /* 
   * 파일 이름에 해당하는 파일을 제거
   * 파일 제거 성공 시 true 반환, 실패 시 false 반환
   */
  check_address(file);
  return filesys_remove(file);
}

int open (const char *file) {
  check_address(file);
  struct thread *cur = thread_current();
  struct file *open_file = filesys_open(file);
  if (open_file) {
    for (int i = 2; i < FD_MAX; i++) {
      if (cur->fdt[i] == NULL) {
        cur->fdt[i] = open_file;
        cur->next_fd = i + 1;
        return i;
      }
    }
    file_close(open_file); // fdt에 넣지 못했으면 file_close 해줘야함. 이거 추가했더니 multi-oom 통과!
  }
  return -1;
}

int filesize (int fd) {
  struct file *file = thread_current()->fdt[fd];
  if (file)
    return file_length(file);
  return -1;
}

int read (int fd, void *buffer, unsigned size) {
  check_address(buffer);
  if (fd == 0) {
    int byte = input_getc();
    return byte;
  }
  struct file *file = thread_current()->fdt[fd];
  if (file) {
    lock_acquire(&filesys_lock);
    int read_byte = file_read(file, buffer, size);
    lock_release(&filesys_lock);
    return read_byte;
  }
  return -1;
}

int write (int fd UNUSED, const void *buffer, unsigned size) {
  check_address(buffer);
  if (fd == 1) {
	  putbuf(buffer, size);
    return size;
  }

  struct file *file = thread_current()->fdt[fd];
  if (file) {
    lock_acquire(&filesys_lock);
    int write_byte = file_write(file, buffer, size);
    lock_release(&filesys_lock);
    return write_byte;
  }
}

void seek (int fd, unsigned position) {
  struct file *curfile = thread_current()->fdt[fd];
  file_seek(curfile, position);
}

unsigned tell (int fd) {
  struct file *curfile = thread_current()->fdt[fd];
  return file_tell(curfile);
}

void close (int fd) {
  struct file * file = thread_current()->fdt[fd];
  if (file) {
    lock_acquire(&filesys_lock);
    file_close(file);
    thread_current()->fdt[fd] = NULL;
    lock_release(&filesys_lock);
  }
}

void check_address(void *addr) {
  struct thread *cur = thread_current();
  if (addr == NULL || is_kernel_vaddr(addr) || pml4_get_page(cur->pml4, addr) == NULL)
    exit(-1);
}
