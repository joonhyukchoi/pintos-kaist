#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"
// #include "userprog/process.c"

struct page;
enum vm_type;

struct file_page {
	uint32_t read_byte;
	uint32_t zero_byte;
	enum vm_type type;
	struct file* file;
	off_t offset;
	size_t swap_slot;
};

void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
void do_munmap (void *va);
#endif
