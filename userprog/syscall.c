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
    case SYS_CREATE:
      f->R.rax = create(f->R.rdi, f->R.rsi);
      break;
    case SYS_REMOVE:
      f->R.rax = remove(f->R.rdi);
      break;
    case SYS_WRITE:      
      f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
      break;
    case SYS_EXEC:
      if(exec(f->R.rdi) == -1) {
        exit(-1);
      }
      break;
    case SYS_WAIT:
      f->R.rax = wait(f->R.rdi);
      break;
    default:
      exit(-1);
      break;
  }

	// printf ("system call!\n");
	// thread_exit ();
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

int wait (tid_t pid) {
  return process_wait(pid);
}


int write (int fd UNUSED, const void *buffer, unsigned size) {
	// temp()
  check_address(buffer);
  if (fd == 1) {
	  putbuf(buffer, size);
	  // return sizeof(buffer);
  }
}

int exec(char *file_name) {
  check_address(file_name);

  int file_size = strlen(file_name) + 1;
  char *fn_copy = palloc_get_page(PAL_ZERO);
  if (fn_copy == NULL)
    exit(-1);
  strlcpy(fn_copy, file_name, file_size);

  if(process_exec(fn_copy) == -1)
    return -1;

  NOT_REACHED();
  return 0;
}

void check_address(void *addr) {
  struct thread *cur = thread_current();
  if (addr == NULL || is_kernel_vaddr(addr) || pml4_get_page(cur->pml4, addr) == NULL)
    exit(-1);
}
