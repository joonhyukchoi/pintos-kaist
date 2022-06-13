#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"

/* pintos project3 */
#include <hash.h>
#include <./threads/mmu.h>

enum vm_type {
	/* page not initialized */
	VM_UNINIT = 0,
	/* page not related to the file, aka anonymous page */
	VM_ANON = 1,
	/* page that realated to the file */
	VM_FILE = 2,
	/* page that hold the page cache, for project 4 */
	VM_PAGE_CACHE = 3,

	/* Bit flags to store state */

	/* Auxillary bit flag marker for store information. You can add more
	 * markers, until the value is fit in the int.
	 * 저장 정보를 위한 보조 비트 플래그 마커.
	 * 값이 int에 맞을 때까지 더 많은 마커를 추가할 수 있다.*/
	VM_MARKER_0 = (1 << 3),
	VM_MARKER_1 = (1 << 4),

	/* DO NOT EXCEED THIS VALUE. */
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type) & 7)

/* The representation of "page".
 * This is kind of "parent class", which has four "child class"es, which are
 * uninit_page, file_page, anon_page, and page cache (project4).
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE. */
struct page {
	const struct page_operations *operations;
	void *va;              /* Address in terms of user space */
	struct frame *frame;   /* Back reference for frame */

	/* Your implementation */
	/* pintos project3 */
    uint8_t type;      /* VM_BIN, VM_FILE, VM_ANON의 타입 */
	bool writable;     /* True일 경우 해당 주소에 write 가능
                          False일 경우 해당 주소에 write 불가능 */
    bool is_loaded;    /* 물리메모리의 탑재 여부를 알려주는 플래그 */
    struct file *vmfile; /* 가상주소와 맵핑된 파일 */
    /* Memory Mapped File 에서 다룰 예정 */
    struct list_elem mmap_elem; /* mmap 리스트 element */
    size_t offset;              /* 읽어야 할 파일 오프셋 */
    size_t read_bytes;          /* 가상페이지에 쓰여져 있는 데이터 크기 */
    size_t zero_bytes;          /* 0으로 채울 남은 페이지의 바이트 */
    /* Swapping 과제에서 다룰 예정 */
    size_t swap_slot; /* 스왑 슬롯 */
    /* ‘vm_entry들을 위한 자료구조’ 부분에서 다룰 예정 */
    struct hash_elem elem; /* 해시 테이블 Element */

	/* Per-type data are binded into the union.
	 * Each function automatically detects the current union */
	union {
		struct uninit_page uninit;
		struct anon_page anon;
		struct file_page file;
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

/* The representation of "frame" */
struct frame {
	void *kva;
	struct page *page;
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

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed.
 * 페이지 작업을 위한 함수 테이블입니다.
 * 이것은 C에서 "인터페이스"를 구현하는 한 가지 방법입니다.
 * "method" 테이블을 구조체의 멤버에 넣고 필요할 때마다 호출합니다. */
struct page_operations {
	bool (*swap_in) (struct page *, void *);
	bool (*swap_out) (struct page *);
	void (*destroy) (struct page *);
	enum vm_type type;
};

#define swap_in(page, v) (page)->operations->swap_in ((page), v)
#define swap_out(page) (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

/* Representation of current process's memory space.
 * We don't want to force you to obey any specific design for this struct.
 * All designs up to you for this. */
struct supplemental_page_table {
	/* pintos project3 */
	struct hash *hash;
};

#include "threads/thread.h"
void supplemental_page_table_init (struct supplemental_page_table *spt);
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src);
void supplemental_page_table_kill (struct supplemental_page_table *spt);
struct page *spt_find_page (struct supplemental_page_table *spt,
		void *va);
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);
void spt_remove_page (struct supplemental_page_table *spt, struct page *page);

void vm_init (void);
bool vm_try_handle_fault (struct intr_frame *f, void *addr, bool user,
		bool write, bool not_present);

#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer ((type), (upage), (writable), NULL, NULL)
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage,
		bool writable, vm_initializer *init, void *aux);
void vm_dealloc_page (struct page *page);
bool vm_claim_page (void *va);
enum vm_type page_get_type (struct page *page);

#endif  /* VM_VM_H */
