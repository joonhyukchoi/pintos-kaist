/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct.
 * file_ops는 파일 지원 페이지에 대한 함수 포인터 테이블 */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm 
 * 파일 지원 페이지 하위 시스템을 초기화합니다.
 * 이 기능에서는 파일 백업 페이지와 관련된 모든 것을 설정할 수 있습니다.*/
void
vm_file_init (void) {
}

/* Initialize the file backed page.
 * 파일 지원 페이지를 초기화합니다. 이 함수는 먼저 page->operation에서 파일 지원 페이지에 대한 핸들러를 설정합니다.
 * 메모리를 지원하는 파일과 같은 페이지 구조에 대한 일부 정보를 업데이트할 수 있습니다. */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;

	/* pintos project3 */
	struct aux_struct *dummy = (struct aux_struct *)(page->uninit.aux);

	file_page->file = dummy->vmfile;
	file_page->offset = dummy->ofs;
	file_page->read_byte = dummy->read_bytes;
	file_page->zero_byte = dummy->zero_bytes;

	if(file_read_at(dummy->vmfile , kva , dummy->read_bytes , dummy->ofs ) != dummy->read_bytes) {

		return false;
	}
	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller.
 * 연결된 파일을 닫아 파일 백업 페이지를 파괴합니다. 내용이 더러우면 변경 사항을 파일에 다시 기록하십시오.
 * 이 함수에서 페이지 구조를 해제할 필요가 없습니다. file_backed_destroy의 호출자가 이를 처리해야 합니다.*/
static void
file_backed_destroy (struct page *page) {
	struct file_page *p UNUSED = &page->file;
	/* 안애매함 */
	do_munmap(page->va);
	
	if (page->frame)
	{
		palloc_free_page(page->frame->kva);
		free(page->frame);
	}
	pml4_clear_page(thread_current()->pml4, page->va);
	free(page);
}

static bool
lazy_load_file (struct page *page, struct aux_struct *aux) {
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */

	/* pintos project3 */
	if (file_read_at(aux->vmfile, page->frame->kva, aux->read_bytes, aux->ofs) != (int) aux->read_bytes) {
	    palloc_free_page (page->frame->kva);
		free(aux);
		return false;
	}
	memset (page->frame->kva + aux->read_bytes, 0, aux->zero_bytes);
	free(aux);

	return true;
}

/* Do the mmap. [+ mmap()이랑 munmap()은 user/syscall.c 안에 ]
 * offset 바이트에서 시작하여 addr에 있는 프로세스의 가상 주소 공간에 fd로 열린 파일의 length 바이트를 매핑합니다.
 * 전체 파일은 addr에서 시작하는 연속적인 가상 페이지에 매핑됩니다.
 * 파일 길이가 PGSIZE의 배수가 아니면 매핑된 최종 페이지의 일부 바이트가 파일 끝을 넘어 "삐져나옵니다".
 * 페이지에 오류가 발생하면 이 바이트를 0으로 설정하고 페이지를 디스크에 다시 쓸 때 버립니다.
 * 성공하면 이 함수는 파일이 매핑된 가상 주소를 반환합니다.
 * 실패하면 파일을 매핑하는 데 유효한 주소가 아닌 NULL을 반환해야 합니다.
 * 
 * + Linux에서 addr이 NULL이면 커널은 매핑을 생성할 적절한 주소를 찾음
 * 그래서 addr이 0이면 일부 Pintos 코드는 가상 페이지 0이 매핑되지 않았다고 가정하기에 실패해야 함.
 * length가 0일 때도 mmap이 실패해야 함.*/
/* pintos project3 */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	
	off_t read_size = file_length(file);
	// off_t zero_size = length - read_size;
	void* va = addr;

	while (0 < read_size){
		struct aux_struct *temp_aux = (struct aux_struct*)malloc(sizeof(struct aux_struct));

        uint32_t read_bytes = read_size > PGSIZE ? PGSIZE : read_size;
		
		temp_aux->vmfile = file;
		temp_aux->ofs = offset;
		temp_aux->read_bytes = read_bytes;
		temp_aux->zero_bytes = PGSIZE - read_bytes;
		temp_aux->writable = writable;
		temp_aux->upage = va;

		if (!vm_alloc_page_with_initializer(VM_FILE, va, writable, lazy_load_file, temp_aux))
			return NULL;
		
		read_size -= read_bytes;
		va += PGSIZE; //애매함 
		offset += read_bytes;
	}
	return addr;
}

/* Do the munmap.
 * 아직 매핑 해제되지 않은 동일한 프로세스에서 mmap에 대한 이전 호출에서
 * 반환된 가상 주소여야 하는 지정된 주소 범위 addr에 대한 매핑을 해제합니다.
 * 
 * + 필요에 따라 vm/vm.c에서 vm_file_init 및 vm_file_initializer를 수정할 수 있음. */
/* pintos project3 */
void
do_munmap (void *addr) {
	struct page *page = spt_find_page(&thread_current()->spt, addr);
	struct file *file = page->file.file;

	off_t read_size = file_length(file);

	while (page = spt_find_page(&thread_current()->spt, addr)){
		if (page->file.file != file) {
			return;
		} 
		if (pml4_is_dirty(thread_current()->pml4, addr)) {
			pml4_set_dirty(thread_current()->pml4, addr, false);
			file_write_at(page->file.file, addr, page->file.read_byte, page->file.offset);
		}	
		addr += PGSIZE;

	}

}
