/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

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

	/* pintos project3 */
	// hash_init(vm, vm_hash_func, vm_less_func, NULL);
}

/* pintos project3 */
static unsigned vm_hash_func (const struct hash_elem *e, void *aux){
    struct page *entry = hash_entry(e, struct page, elem);

    return hash_int((int)entry->va);
}

/* pintos project3 */
static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b){
    struct page *a_entry = hash_entry(a, struct page, elem);
    struct page *b_entry = hash_entry(b, struct page, elem);

    return (int)a_entry->va < (int)b_entry->va;
}

/* pintos project3 */
bool insert_vme (struct hash *vm, struct page *vme){
	if (hash_insert(vm, &vme->elem)){
		return false;
	}
	return true;	
}

/* pintos project3 */
bool delete_vme (struct hash *vm, struct page *vme){
	if (hash_delete(vm, &vme->elem)){
		return true;
	}
	return false;
}

/* pintos project3 */
struct page *find_vme (void *vaddr){
	// struct hash *hash_table = &((struct thread *)pg_round_down(vaddr))->vm;
	struct hash_elem search_elem = ((struct page *)pg_round_down(vaddr))->elem;
	struct thread *curr = thread_current();

	struct hash_elem *found_elem = hash_find(&curr->vm, &search_elem);

	if (!found_elem){
		return NULL;
	}

	return hash_entry(found_elem, struct vm_entry, elem);
}

/* pintos project3 */
void vm_destroy (struct hash *vm){
	// switch (expression)
	// {
	// case /* constant-expression */:
	// 	/* code */
	// 	break;
	// // vm_dealloc_page();
	// default:
	// 	break;
	// }
	// hash_destroy(vm, );
}

/* pintos project3 */
void vm_destroy_func (struct hash_elem *e, void *aux){
	struct vm_entry *vm_entry_destroy = hash_entry(e, struct vm_entry, elem);

	// if (vm_entry_destroy->is_loaded){
	// 	palloc_free_page(vm_entry_destroy);
	// 	pml4_clear_page();
	// }
	// palloc_free_page(vm_entry_destroy);
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

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new.
		 * 페이지를 생성하고 VM type에 따라 이니셜을 가져온 다음 uninit_new를 호출하여
		 * "uninit" 페이지 구조체를 생성합니다. unitit_new를 호출한 후 필드를 수정해야 합니다. */
		struct page *newpage = malloc(sizeof(struct page));
		
		if (type == 1) {
			uninit_new(newpage, upage, init, type, aux, anon_initializer);
		} else if (type == 2) {
			uninit_new(newpage, upage, init, type, aux, file_backed_initializer);
			newpage->vmfile = ((struct aux_struct *)aux)->vmfile;
			newpage->offset = ((struct aux_struct *)aux)->ofs;
			newpage->read_bytes = ((struct aux_struct *)aux)->read_bytes;
			newpage->zero_bytes = ((struct aux_struct *)aux)->zero_bytes;
			newpage->is_loaded = ((struct aux_struct *)aux)->is_loaded;
		}
		newpage->is_loaded = false;
		newpage->type = type;
		newpage->writable = writable;
		/* 오프셋 애매함 */
		// upage - pg_round_down(upage)
		
		/* TODO: Insert the page into the spt.
		 * 페이지를 spt에 삽입하십시오. */
		spt_insert_page(spt, newpage);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
/* 애매함 */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/* TODO: Fill this function. */
	struct page page;
	struct thread *curr = thread_current();
	page.va = pg_round_down(va);
	struct hash_elem *found_elem = hash_find(&curr->vm, &page.elem);

	if (!found_elem){
		return NULL;
	}

	return hash_entry(found_elem, struct page, elem);
	
	// return page;
}


/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	if (hash_insert(page, &page->elem)){
		return false;
	}
	return true;	
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
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	frame = malloc(sizeof(struct frame));
	frame->kva = palloc_get_page(PAL_USER);

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	if (!frame->kva){
		free(frame);
		PANIC("todo");
	}

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
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
/* 애매함 */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = malloc(sizeof(struct page));
	/* TODO: Fill this function */
	page->va = va;
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	if (spt_insert_page(thread_current()->spt, page))
		return swap_in (page, frame->kva);
	free(page);
	free(frame);
	return false;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(spt->hash, vm_hash_func, vm_less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
