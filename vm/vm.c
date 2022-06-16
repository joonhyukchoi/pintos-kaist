/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

static unsigned vm_hash_func (const struct hash_elem *e, void *aux);
static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b);
static bool vm_do_claim_page (struct page *page);

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* pintos project3 */
static unsigned vm_hash_func (const struct hash_elem *e, void *aux){
    struct page *entry = hash_entry(e, struct page, elem);

    return hash_bytes(&entry->va, sizeof(entry->va));
}

/* pintos project3 */
static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b){
    struct page *a_entry = hash_entry(a, struct page, elem);
    struct page *b_entry = hash_entry(b, struct page, elem);

    return a_entry->va < b_entry->va;
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

    bool success = false;
	struct supplemental_page_table *spt = &thread_current()->spt;

    /* Check wheter the upage is already occupied or not. */
    if (spt_find_page(spt, upage) == NULL)
    {
        /* TODO: Create the page, fetch the initialier according to the VM type,
         * TODO: and then create "uninit" page struct by calling uninit_new. You
         * TODO: should modify the field after calling the uninit_new.
         * 페이지를 생성하고 VM type에 따라 이니셜을 가져온 다음 uninit_new를 호출하여
         * "uninit" 페이지 구조체를 생성합니다. unitit_new를 호출한 후 필드를 수정해야 합니다. */

        /* pintos project3 */
        struct page *p = (struct page *)malloc(sizeof(struct page));

        if (type == VM_ANON)
            uninit_new(p, pg_round_down(upage), init, type, aux, anon_initializer);
        else if (type == VM_FILE)
            uninit_new(p, pg_round_down(upage), init, type, aux, file_backed_initializer);

        p->writable = writable;
        /* TODO: Insert the page into the spt. */
        success = spt_insert_page(spt, p);
    }
    return success;
err:
    return success;
    // return false;
}

/* Find VA from spt and return page. On error, return NULL. */
/* 애매함 */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/* TODO: Fill this function. */
	/* pintos project3 */
	struct page page;
	page.va = pg_round_down(va);
	struct hash_elem *found_elem = hash_find(&spt->hash, &(page.elem));

	if (!found_elem){
		return NULL;
	}

	return hash_entry(found_elem, struct page, elem);
}


/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/* pintos project3 */
    int success = false;
	/* TODO: Fillthis function. */
	if(hash_insert (&(spt->hash), &(page->elem)) == NULL){
		success = true;
	}
	return success;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space. 
 * palloc() 프레임을 가져옵니다. 사용 가능한 페이지가 없으면 페이지를 제거하고 반환합니다.
 * 이것은 항상 유효한 주소를 반환합니다.
 * 즉, 사용자 풀 메모리가 가득 차면 이 함수는 사용 가능한 메모리 공간을 얻기 위해 프레임을 축출합니다. */
static struct frame *
vm_get_frame (void) {
    struct frame *frame = (struct frame*)malloc(sizeof(struct frame));

	/* TODO: Fill this function. */
	/* pintos project3 */
    frame->kva = palloc_get_page(PAL_USER);
	frame->page = NULL;

    if (frame->kva == NULL){
        frame = NULL;
		PANIC("todo");
	}

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);

    return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	
	/* pintos project3 */
	struct page *page = spt_find_page(spt, addr);
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	/* pintos project3 */
	if (page == NULL){
		return false;
	}

	if (page && not_present){
		return vm_do_claim_page(page);
	} else {
        return false;
    }
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA.
 * VA를 할당할 페이지를 요청합니다.
 * 먼저 페이지를 가져온 다음 해당 페이지와 함께 vm_do_claim_page를 호출해야 합니다. */
/* 애매함 */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = spt_find_page(&thread_current()->spt, va);
	/* TODO: Fill this function */
	/* pintos project3 */

    if (page == NULL){
        return false;
    }
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu.
 * 클레임은 물리적 프레임인 페이지를 할당하는 것을 의미합니다.
 * 먼저 vm_get_frame(템플릿에서 이미 수행됨)을 호출하여 프레임을 얻습니다.
 * 그런 다음 MMU를 설정해야 합니다.
 * 즉, 가상 주소에서 페이지 테이블의 물리적 주소로의 매핑을 추가합니다.
 * 반환 값은 작업의 성공 여부를 나타내야 합니다. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;
	
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* pintos project3 */
	pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable);
	// if(page_get_type(page) && VM_MARKER_0)
	// {
	// 	return true;
	// }

    return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	/* pintos project3 */
	hash_init(&spt->hash, vm_hash_func, vm_less_func, NULL);
}

/* pintos project3 */
void copy_page (struct hash_elem *e, void *aux)
{
	struct page* page = hash_entry(e, struct page , elem);

	vm_alloc_page(page->uninit.type , page->va , page->writable);
	if(page->frame){
		struct page *child = spt_find_page(&thread_current()->spt ,page->va);
		child->frame = vm_get_frame();
		memcpy(child->frame->kva, page->frame->kva, PGSIZE);
		child->frame->page = child;
		pml4_set_page(thread_current()->pml4, child->va, child->frame->kva, child->writable);
	}
	
	// vm_alloc_page_with_initializer(page_get_type(page) | VM_MARKER_1, page->va , page->writable,NULL,page);

	// struct page *child = (struct page*) malloc(sizeof page);
	// memcpy(child , page, sizeof page);
	// spt_insert_page(thread_current()->spt, child);
	// struct aux_struct *temp_aux = (struct aux_struct*)malloc(sizeof(struct aux_struct));

	// 	temp_aux->vmfile = page->vmfile;
	// 	temp_aux->ofs = page->offset;
	// 	temp_aux->read_bytes = page->read_bytes;
	// 	temp_aux->zero_bytes = page->zero_bytes;
	// 	temp_aux->writable = page->writable;
	// 	temp_aux->upage = page->va;

	// if (page->operations->type == VM_UNINIT) {
	// 	struct page *p = (struct page *)malloc(sizeof(struct page));
	// 	memcpy(p, page, sizeof page);
	// 	// uninit_new();
	// 	spt_insert_page(&thread_current()->spt, p);
	// 	printf("dwdwd\n");
	// } else {
	// 	vm_alloc_page(page->operations->type , page->va , page->writable);
	// 	printf("wgrgergegre -2 \n");
	// }
	// vm_alloc_page_with_initializer(page->type , page->va , page->writable, NULL , NULL);
}

/* Copy supplemental page table from src to dst.
 * src에서 dst로 보조 페이지 테이블을 복사합니다.
 * 이것은 자식이 부모의 실행 context를 상속해야 할 때 사용됩니다(예: fork()).
 * src의 추가 페이지 테이블에 있는 각 페이지를 반복하고 dst의 추가 페이지 테이블에 있는
 * 항목의 정확한 복사본을 만듭니다. unitit page를 할당하고 즉시 claim 해야 합니다. */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	// hash_first(&init, src->hash);
	/* src  테이블에서 모든 page 구조체를  dst 테이블로 복사*/
	/* 타입이 uninit, anon, file 얘네를 다 uninit으로 할당 */
	hash_apply(&src->hash, copy_page);
	return true; // 무조건 true의 문제??
}

/* pintos project3 */
void kill_page (struct hash_elem *e, void *aux){
	struct page *page = hash_entry(e, struct page , elem);
	destroy(page);
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// printf("supplemental_page_table_kill 되냐?\n");
	hash_clear(&spt->hash, kill_page);
}
