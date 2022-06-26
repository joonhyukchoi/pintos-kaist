// * USERPROG 추가
#include <stdbool.h>
#include "threads/thread.h"

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct lock filesys_lock;

void syscall_init (void);

// * 추가
void check_address(void *addr);

// * syscall 추가
void halt(void);
void exit(int status);
int fork (const char *thread_name);
int exec (const char *file_name);
int wait (tid_t pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

/* pintos project3 */
void check_valid_string (const void *str, unsigned size);
void check_valid_buffer (void *buffer, unsigned size, bool to_write);
void *mmap (void *addr, size_t length, int writable, int fd, int offset);
void munmap (void *addr);

/* pintos project4 */
bool chdir (const char *dir);
bool mkdir (const char *dir);
bool readdir (int fd, char *name);
bool isdir (int fd);
int inumber (int fd);
int symlink (const char *target, const char *linkpath);

#endif /* userprog/syscall.h */
