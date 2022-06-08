/* pintos project3 */

#include <page.h>

void vm_init (struct hash *vm){
    hash_init(vm, vm_hash_func, vm_less_func, NULL);
}

static unsigned vm_hash_func (const struct hash_elem *e, void *aux){
    struct vm_entry *entry = hash_entry(e, struct vm_entry, elem);

    return hash_int((int)entry->vaddr);
}

static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b){
    struct vm_entry *a_entry = hash_entry(a, struct vm_entry, elem);
    struct vm_entry *b_entry = hash_entry(b, struct vm_entry, elem);

    return (int)a_entry->vaddr < (int)b_entry->vaddr;
}