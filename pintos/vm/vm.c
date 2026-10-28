/* vm.c: 가상 메모리 객체의 공통 인터페이스. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"

uint64_t hash_find_func (const struct hash_elem *e, void *aux);
bool page_compare_func (const struct hash_elem *a, const struct hash_elem *b, void *aux);


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

/**
 * @brief 주어진 가상 주소가 속한 페이지의 관리 객체를 spt에서 찾음
 * 
 * @details
 * 입력 주소를 해당 페이지의 시작 주소로 맞춘 뒤 검색
 * 예를 들어 0x5123이 들어오면 0x5000을 검색 키로 사용
 * 
 * 임시 page(tmp) 객체는 검색할 키를 전달하는 용도로만 사용
 * 검색에 성공하면 spt에 등록되어 있던 실제 페이지 객체를 반환
 */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) {
	struct page *page = NULL;
	/* TODO: 이 함수를 구현할 것. */
	struct page tmp;
	tmp.va = pg_round_down(va);		// 인자로 받은 va의 페이지 시작 주소

	struct hash_elem *tmp_hash_elem = hash_find(&spt->hash, &tmp.hash_elem);
	if (tmp_hash_elem == NULL) return NULL;

	page = hash_entry(tmp_hash_elem, struct page, hash_elem);

	return page;
}

/**
 * @brief 페이지 관리 객체를 spt에 등록
 * 
 * @details
 * 페이지에 포함된 hash_elem을 해시 테이블에 연결
 * 같은 시작 가상 주소를 가진 페이지가 이미 있으면
 * 기존 페이지를 유지하고 새 페이지는 등록하지 않음
 */
bool
spt_insert_page (struct supplemental_page_table *spt, struct page *page) {
	int succ = false;
	/* TODO: 이 함수를 구현할 것. */
	struct hash_elem *res = hash_insert(&spt->hash, &page->hash_elem);
	
	if (!res) {
		succ = true;
	}

	return succ;
}

/**
 * @brief spt에서 페이지를 제거하고 관리 객체 해제
 * 
 * @details
 * 해시 테이블에서 페이지의 연결 고리를 먼저 제거
 * 이후 vm_dealloc_page()를 호출하여 페이지별 정리 함수를 실행하고,
 * 관리 객체의 메모리 해제
 */
void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	hash_delete(&spt->hash, &page->hash_elem);
	vm_dealloc_page (page);
	
	return;
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
/**
 * @brief 사용자 데이터를 담을 빈 프레임 준비
 * 
 * @details
 * malloc()으로 struct frame 관리 객체를 할당하고,
 * 사용자 풀에서 실제 데이터를 담을 4KB 공간 확보
 * 확보한 공간의 커널 가상 주소를 frame->kva에 저장하고,
 * 아직 연결된 페이지가 없으므로 frame->page를 NULL로 설정
 * 
 * @note 현재 단계에서는 메모리 확보에 실패하면 PANIC으로 중단
 * 	     사용자풀 할당에 실패하면 먼저 할당한 관리 객체 해제함
 */
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: 이 함수를 구현할 것. */
	frame = malloc(sizeof *frame);
	if (frame == NULL) {
		PANIC("todo");
	}

	frame->kva = palloc_get_page(PAL_USER);
	if (frame->kva == NULL) {
		free(frame);
		PANIC("todo");
	}

	frame->page = NULL;
	
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
vm_claim_page (void *va) {
	struct page *page = NULL;
	struct thread *thread = thread_current();
	/* TODO: 이 함수를 구현할 것. */
	page = spt_find_page(&thread->spt, va);
	if (page == NULL) {
		return false;
	}
	
	return vm_do_claim_page (page);
}

/* PAGE의 물리 프레임을 확보하고 MMU를 설정함. */
static bool
vm_do_claim_page (struct page *page) { 
	struct frame *frame = vm_get_frame ();

	/* 서로를 가리키는 참조를 연결함. */
	frame->page = page;
	page->frame = frame;

	uint64_t *pml4 =  thread_current()->pml4;

	/* TODO: 페이지의 VA를 프레임의 PA에 매핑하는 페이지 테이블 항목을 삽입할 것. */
	bool result = false;
	result = pml4_set_page(pml4, page->va, frame->kva, page->writable);
	bool swap_result = false;

	if (result == true) {
		swap_result = swap_in (page, frame->kva);
		if (swap_result == true) {
			return true;
		} else {
			page->frame = NULL;
			pml4_clear_page(pml4, page->va);
			palloc_free_page(frame->kva);
			free(frame);
			return false;
		}
	} else {
		page->frame = NULL;
		palloc_free_page(frame->kva);
		free(frame);
		return false;
	}
	
}

/* 새 보조 페이지 테이블을 초기화함. */
void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	bool result = hash_init(&spt->hash, hash_find_func, page_compare_func, NULL);

	if (!result) {
		PANIC("Fail to hash table initd\n");
	}
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

/**
 * @brief 페이지의 시작 가상 주소로 해시 값을 계산
 * @details
 * 전달받은 연결 고리 e에서 이를 포함하는 struct page를 찾음
 * 그 페이지의 va에 저장된 주소 값을 해시로 계산
 * 
 * 페이지 안의 실제 데이터나 관리 객체 자체의 주소는 해시 계산에 사용 X
 */
uint64_t hash_find_func (const struct hash_elem *e, void *aux) {
	struct page* page_address = hash_entry(e, struct page, hash_elem);

	return hash_bytes(&page_address->va, sizeof page_address->va);
}

/**
 * @brief 첫 번째 페이지의 시작 가상 주소가 두 번째보다 작은지 확인
 * 
 * @details
 * 연결 고리 a와 b에서 각각의 struct page를 찾고,
 * 두 페이지의 va에 저장된 주소 값을 비교
 * 
 * 해시 라이브러리는 이 함수를 양방향으로 호출
 * A가 B보다 작지도 않고, B가 A보다 작지도 않으면 두 요소를 같은 키로 판단
 */
bool page_compare_func (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	struct page* a_page_ptr = hash_entry(a, struct page, hash_elem);
	struct page* b_page_ptr = hash_entry(b, struct page, hash_elem);

	if (a_page_ptr->va >= b_page_ptr->va) {
		return false;
	}
	else return true;
}