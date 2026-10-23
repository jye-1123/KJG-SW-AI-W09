#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"

enum vm_type {
	/* 아직 초기화되지 않은 페이지. */
	VM_UNINIT = 0,
	/* 파일과 연결되지 않은 페이지, 즉 익명 페이지. */
	VM_ANON = 1,
	/* 파일과 연결된 페이지. */
	VM_FILE = 2,
	/* 페이지 캐시를 담는 페이지. Project 4에서 사용함. */
	VM_PAGE_CACHE = 3,

	/* 상태를 저장하는 비트 플래그. */

	/* 정보를 저장하기 위한 보조 비트 플래그 마커.
	 * 값이 int 범위에 들어가는 한 마커를 더 추가할 수 있음. */
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

/* "page"를 나타내는 구조체.
 * uninit_page, file_page, anon_page, 페이지 캐시(Project 4)를
 * 자식 클래스로 두는 부모 클래스에 해당함.
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE. */
struct page {
	const struct page_operations *operations;
	void *va;              /* 사용자 공간의 가상 주소. */
	struct frame *frame;   /* 프레임에 대한 참조. */

	/* 필요한 구현을 추가할 위치. */

	/* 유형별 데이터를 union에 묶어 저장함.
	 * 각 함수가 현재 사용하는 union 멤버를 자동으로 판별함. */
	union {
		struct uninit_page uninit;
		struct anon_page anon;
		struct file_page file;
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

/* "frame"을 나타내는 구조체. */
struct frame {
	void *kva;
	struct page *page;
};

/* 페이지 연산의 함수 표.
 * C에서 인터페이스를 구현하는 한 가지 방법임.
 * 메서드의 함수 표를 구조체 멤버에 넣고,
 * 필요할 때마다 이를 통해 호출함. */
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

/* 현재 프로세스의 메모리 공간을 나타냄.
 * 이 구조체에 특정 설계를 강제하지 않음.
 * 모든 설계는 자유롭게 결정할 수 있음. */
struct supplemental_page_table {
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
