#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
struct page;
enum vm_type;

struct anon_page {
    // 번역 = 익명 페이지의 상태 또는 필요한 정보를 저장하기 위한 구성원을 추가할 수 있다.
    // 페이지의 일반 정보가 담긴 struct page를 참고해도 괜춘타
    // you may add members to store necessary information
    // or state of an anonymous page as you implement.
    /* see the struct page in include/vm/page.h,
    which contains generic information of a page. */
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif
