#include <stdbool.h>

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

// * 추가
void check_address(void *addr);

// * syscall 추가
void halt(void);
void exit(int status);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int write(int fd, const void *buffer, unsigned size);

int exec(char *file_name);

#endif /* userprog/syscall.h */
