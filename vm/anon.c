/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages.
 * + 이 기능에서는 스왑 디스크를 설정해야 합니다.
 * 또한 스왑 디스크에서 사용 가능한 영역과 사용된 영역을 관리하기 위한 데이터 구조가 필요합니다.
 * 스왑 영역도 PGSIZE(4096바이트) 단위로 관리됩니다.*/
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	/* pintos project3 */
	swap_disk = disk_get(1, 1);
	/* disk sector 크기 = 512bytes
	 * page 크기 = 4096bytes = 512*8 bytes = disk sector*8 */
	thread_current()->disk_table = bitmap_create(disk_size(swap_disk) / 8);
}

/* Initialize the file mapping.
 * + 이것은 anonymous page의 initializer입니다.
 * 스와핑을 지원하려면 anon_page에 몇 가지 정보를 추가해야 합니다.*/
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk.
 * + 디스크에서 메모리로 데이터 내용을 읽어 swap disk에서 anonymous page로 swap합니다.
 * 데이터의 위치는 페이지가 swap out될 때 page struct에 swap disk가 저장되어 있어야 합니다.
 * swap table을 업데이트하는 것을 잊지 마십시오(Managing the Swap Table 참조). */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;

	/* swap out에서 매핑을 해제하게 된다면 여기서 할당해줘야 함 */

	/* pintos project3 */
	struct disk *cur_disk = thread_current()->disk_table;
	size_t sector_slot = page->anon.swap_slot;
	disk_read(cur_disk, sector_slot, kva);
	bitmap_set(cur_disk, sector_slot, false);
}

/* Swap out the page by writing contents to the swap disk.
 * + 메모리에서 디스크로 내용을 복사하여 anonymous page를 swap disk로 교체합니다.
 * 먼저 swap table을 사용하여 디스크에서 free swap slot을 찾은 다음 데이터 page를 슬롯에 복사합니다.
 * 데이터의 위치는 page struct에 저장해야 합니다. 디스크에 free slot이 더 이상 없으면, you can panic the kernel. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;

	/* pintos project3 */
	/* 해시테이블 순회해서 swap_slot = 1이 아닌거를 찾아서 */
	struct disk *cur_disk = thread_current()->disk_table;

	size_t empty_slot = bitmap_scan(cur_disk, 0, 1, false);
	
	if (empty_slot == BITMAP_ERROR){
		PANIC("empty_slot == BITMAP_ERROR");
	}
	page->anon.swap_slot = empty_slot;
	disk_write(cur_disk, empty_slot, page->frame->kva);
	bitmap_set(cur_disk, empty_slot, true);
	/* 물리 메모리 매핑 해제 해야함=?? */

}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	if (page->frame)
	{
		palloc_free_page(page->frame->kva);
		free(page->frame);
	}
	pml4_clear_page(thread_current()->pml4, page->va);
	free(page);
}
