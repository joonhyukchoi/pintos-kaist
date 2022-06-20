/* pintos project3 */

#include <stdint.h>
#include <stdbool.h>
#include <hash.h>

#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

struct vm_entry
{
    uint8_t type;      /* VM_BIN, VM_FILE, VM_ANON의 타입 */
    void *vaddr;       /* vm_entry가 관리하는 가상페이지 번호 */
    bool writable;     /* True일 경우 해당 주소에 write 가능
                          False일 경우 해당 주소에 write 불가능 */
    bool is_loaded;    /* 물리메모리의 탑재 여부를 알려주는 플래그 */
    struct file *file; /* 가상주소와 맵핑된 파일 */
    /* Memory Mapped File 에서 다룰 예정 */
    struct list_elem mmap_elem; /* mmap 리스트 element */
    size_t offset;              /* 읽어야 할 파일 오프셋 */
    size_t read_bytes;          /* 가상페이지에 쓰여져 있는 데이터 크기 */
    size_t zero_bytes;          /* 0으로 채울 남은 페이지의 바이트 */
    /* Swapping 과제에서 다룰 예정 */
    size_t swap_slot; /* 스왑 슬롯 */
    /* ‘vm_entry들을 위한 자료구조’ 부분에서 다룰 예정 */
    struct hash_elem elem; /* 해시 테이블 Element */
}

void vm_init (struct hash *vm);
static unsigned vm_hash_func (const struct hash_elem *e, void *aux);
static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b);