#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"

/* pintos project3 */
#include <bitmap.h>

struct page;
enum vm_type;

struct anon_page {
    /* see the struct page in include/vm/page.h,
    which contains generic information of a page. */

    /* pintos project3 */
    size_t swap_slot;
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif
