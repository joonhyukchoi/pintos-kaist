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
#include "include/vm/vm.h"

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
      memcpy(&thread_current()->ptf, f, sizeof(struct intr_frame));
      f->R.rax = (uint64_t)fork(f->R.rdi);
      break;
    case SYS_CREATE:
      f->R.rax = (uint64_t)create(f->R.rdi, f->R.rsi);
      break;
    case SYS_REMOVE:
      f->R.rax = (uint64_t)remove(f->R.rdi);
      break;
    case SYS_OPEN:
      f->R.rax = (uint64_t)open(f->R.rdi);
      break;
    case SYS_FILESIZE:
      f->R.rax = (uint64_t)filesize(f->R.rdi);
      break;
    case SYS_READ:
      f->R.rax = (uint64_t)read(f->R.rdi, f->R.rsi, f->R.rdx);
      break;  
    case SYS_WRITE:
      f->R.rax = (uint64_t)write(f->R.rdi, f->R.rsi, f->R.rdx);
      break;
    case SYS_EXEC:
      exec(f->R.rdi);
      break;
    case SYS_WAIT:
      f->R.rax = (uint64_t)wait(f->R.rdi);
      break; 
    case SYS_SEEK:
      seek(f->R.rdi, f->R.rsi);
      break;
    case SYS_TELL:
      f->R.rax = (uint64_t)tell(f->R.rdi);
      break;
    case SYS_CLOSE:
      close(f->R.rdi);
      break;
    case SYS_MMAP:
      f->R.rax = mmap(f->R.rdi, f->R.rsi, f->R.rdx, f->R.r10, f->R.r8);
      break;
    case SYS_MUNMAP:
      munmap(f->R.rdi);
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

int fork (const char *thread_name) {
  check_address(thread_name);
  return process_fork(thread_name, &thread_current()->ptf);
}

int exec (const char *file_name) {
  check_address(file_name);

  int file_size = strlen(file_name) + 1;
  char *fn_copy = palloc_get_page(PAL_ZERO);
  if (!fn_copy) {
    exit(-1);
    return -1;
  }
  strlcpy(fn_copy, file_name, file_size);
  if (process_exec(fn_copy) == -1) {
    exit(-1);
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
  struct file *fd = filesys_open(file);
  if (fd) {
    for (int i = 2; i < 128; i++) {
      if (!cur->fdt[i]) {
        cur->fdt[i] = fd;
        cur->next_fd = i + 1;
        return i;
      }
    }
    file_close(fd);
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
  // check_address(buffer);
  check_valid_buffer (buffer, size, true);
  if (fd == 1) {
    return -1;
  }

  if (fd == 0) {
    lock_acquire(&filesys_lock);
    int byte = input_getc();
    lock_release(&filesys_lock);
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
  // check_address(buffer);
  check_valid_string(buffer, size);

  if (fd == 0) // STDIN일 때 -1
    return -1;

  if (fd == 1) {
    lock_acquire(&filesys_lock);
	  putbuf(buffer, size);
    lock_release(&filesys_lock);
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
  if (curfile)
    file_seek(curfile, position);
}

unsigned tell (int fd) {
  struct file *curfile = thread_current()->fdt[fd];
  if (curfile)
    return file_tell(curfile);
}

void close (int fd) {
  struct file * file = thread_current()->fdt[fd];
  if (file) {
    lock_acquire(&filesys_lock);
    thread_current()->fdt[fd] = NULL;
    file_close(file);
    lock_release(&filesys_lock);
  }
}

/* pintos project2 add
 * (+fix) pintos project3 */
void check_address(void *addr)
{
  struct thread *cur = thread_current();
#ifdef VM
  if (addr == NULL || is_kernel_vaddr(addr) || spt_find_page(&cur->spt, addr) == NULL)
    exit(-1);
#else
  if (addr == NULL || is_kernel_vaddr(addr) || pml4_get_page(cur->pml4, addr) == NULL)
    exit(-1);
#endif
}

/* pintos project3 */
void check_valid_string (const void *str, unsigned size) {
  check_address(str);
  struct thread *cur = thread_current();

  for (int i = 0; i < size; i += PGSIZE)
  {
    #ifdef VM
    struct page *p = spt_find_page(&cur->spt, str + i);
    if (p == NULL)
    {
      // printf("check valid buffer writable %d, p->writable %d\n", writable, p->writable);
      exit(-1);
    }
    #endif
  }
}

/* pintos project3 */
void check_valid_buffer (void *buffer, unsigned size, bool to_write) {
  // size PGSIZE
  check_address(buffer);
  struct thread *cur = thread_current();

  for (int i = 0; i < size; i += PGSIZE)
  {
    #ifdef VM
    struct page *p = spt_find_page(&cur->spt, buffer + i);
    if (!p->writable)
    {
      // printf("check valid buffer writable %d, p->writable %d\n", writable, p->writable);
      exit(-1);
    }
    #endif
  }
}

/* pintos project3 */
void *mmap (void *addr, size_t length, int writable, int fd, off_t offset) {
  /* kernel address 일때 exit 인지 불명확*/
  if ((signed)length <= 0 || length < offset) {
    return NULL;
  }

  if (!addr|| fd < 2 || is_kernel_vaddr(addr) || pg_ofs(addr)) {
    return NULL;
  }

  struct file* file = file_reopen(thread_current()->fdt[fd]);
  #ifdef VM
  return do_mmap(addr, length, writable, file, offset);
  #endif
}

/* pintos project3 */
void munmap (void *addr) {
  if (is_kernel_vaddr(addr)) {
    return NULL;
  }
  #ifdef VM
  do_munmap(addr);
  #endif
}

/* pintos project4

 * 프로세스의 현재 작업 디렉터리를 상대 또는 절대 디렉터리로 변경합니다.
 * 성공하면 true를, 실패하면 false를 반환합니다. */
bool chdir (const char *dir){

}

/* pintos project4

 * 상대 또는 절대일 수 있는 dir이라는 디렉터리를 만듭니다.
 * 성공하면 true를, 실패하면 false를 반환합니다.
 * dir이 이미 존재하거나 dir의 마지막 이름 외에 디렉터리 이름이 이미 존재하지 않는 경우 실패합니다.
 * 즉, mkdir("/a/b/c")은 /a/b가 이미 있고 /a/b/c가 없는 경우에만 성공합니다. */
bool mkdir (const char *dir){

}

/* pintos project4

 * 디렉토리를 나타내야 하는 파일 설명자 fd에서 디렉토리 항목을 읽습니다.
 * 성공하면 READDIR_MAX_LEN + 1바이트를 위한 공간이 있어야 하는 이름에 null로
 * 끝나는 파일 이름을 저장하고 true를 반환합니다.
 * 디렉토리에 항목이 남아 있지 않으면 false를 반환합니다.
 * 
 * . 그리고 ..는 readdir에 의해 반환되어서는 안됩니다.
 * 디렉토리가 열려 있는 동안 변경되면 일부 항목이 전혀 읽히지 않거나 여러 번 읽는 것이 허용됩니다.
 * 그렇지 않으면 각 디렉토리 항목은 순서에 관계없이 한 번만 읽어야 합니다.
 * 
 * READDIR_MAX_LEN은 lib/user/syscall.h에 정의되어 있습니다.
 * 파일 시스템이 기본 파일 시스템보다 긴 파일 이름을 지원하는 경우
 * 이 값을 기본값인 14에서 늘려야 합니다. */
bool readdir (int fd, char *name){

}

/* pintos project4

 * fd가 디렉토리를 나타내는 경우 true를 반환하고 일반 파일을 나타내는 경우 false를 반환합니다. */
bool isdir (int fd){

}

/* pintos project4

 * 일반 파일이나 디렉토리를 나타낼 수 있는 fd와 관련된 inode의 inode 번호를 반환합니다.

 * inode 번호는 파일이나 디렉토리를 지속적으로 식별합니다. 파일이 존재하는 동안 고유합니다.
 * Pintos에서 inode의 sector number는 inode number로 사용하기에 적합합니다. */
int inumber (int fd){

}

/* pintos project4
 * 문자열 target을 포함하는 linkpath라는 symbolic link를 만듭니다.
 * 성공하면 0이 반환됩니다. 그렇지 않으면 -1이 반환됩니다. */
int symlink (const char *target, const char *linkpath){

}