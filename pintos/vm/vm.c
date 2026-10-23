/* vm.c: 가상 메모리 객체의 공통 인터페이스. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/* 각 하위 시스템의 초기화 코드를 호출하여 가상 메모리 하위 시스템을 초기화함.
 */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: 여기에 코드를 작성할 것. */
}

/* 페이지의 유형을 반환함. 초기화 후의 페이지 유형을 알고 싶을 때 유용함.
 * 이 함수는 이미 완전히 구현되어 있음.
 */
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

/* 도우미 함수. */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* 초기화 함수를 지정하여 초기화 대기 상태의 페이지 객체를 생성함.
 * 페이지를 직접 생성하지 말고 이 함수나 vm_alloc_page를 통해 생성할 것.
 */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* upage가 이미 사용 중인지 확인함. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: 페이지를 생성하고 VM 유형에 맞는 초기화 함수를 구한 뒤,
		 * TODO: uninit_new를 호출하여 미초기화 페이지 구조체를 생성할 것.
		 * TODO: uninit_new 호출 후 필요한 필드를 수정할 것. */

		/* TODO: 페이지를 spt에 삽입할 것. */
	}
err:
	return false;
}

/* spt에서 VA에 해당하는 페이지를 찾아 반환함. 오류 시 NULL을 반환함. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: 이 함수를 구현할 것. */

	return page;
}

/* 유효성을 확인한 뒤 PAGE를 spt에 삽입함. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: 이 함수를 구현할 것. */

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* 내보낼 대상의 struct frame을 반환함. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: 페이지 교체 정책은 자유롭게 결정할 것. */

	return victim;
}

/* 페이지 하나를 내보내고 해당 프레임을 반환함.
 * 오류 시 NULL을 반환함. */
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: victim을 스왑 아웃하고 비워진 프레임을 반환할 것. */

	return NULL;
}

/* palloc()으로 페이지를 할당하고 프레임을 얻음. 빈 페이지가 없으면
 * 기존 페이지를 내보내 확보한 프레임을 반환함. 항상 유효한 주소를 반환함.
 * 즉, 사용자 풀이 가득 차면 프레임을 비워 사용할 메모리 공간을 확보함.
 */
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: 이 함수를 구현할 것. */

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* 스택을 확장함. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* 쓰기 보호된 페이지에서 발생한 폴트를 처리함. */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* 성공하면 true를 반환함. */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: 처리 가능한 폴트인지 확인할 것. */
	/* TODO: 여기에 코드를 작성할 것. */

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* VA에 할당할 페이지의 물리 프레임을 확보함(claim). */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: 이 함수를 구현할 것. */

	return vm_do_claim_page (page);
}

/* PAGE의 물리 프레임을 확보하고 MMU를 설정함. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* 서로를 가리키는 참조를 연결함. */
	frame->page = page;
	page->frame = frame;

	/* TODO: 페이지의 VA를 프레임의 PA에 매핑하는 페이지 테이블 항목을 삽입할 것. */

	return swap_in (page, frame->kva);
}

/* 새 보조 페이지 테이블을 초기화함. */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
}

/* 보조 페이지 테이블을 src에서 dst로 복사함. */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* 보조 페이지 테이블이 보유한 자원을 해제함. */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: 스레드가 보유한 보조 페이지 테이블을 모두 정리하고,
	 * TODO: 수정된 내용을 저장장치에 다시 기록할 것. */
}
