/* anon.c: 디스크 기반이 아닌 메모리 이미지의 페이지(익명 페이지) 구현. */

#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* 익명 페이지에 필요한 데이터를 초기화함. */
void
vm_anon_init (void) {
	/* TODO: swap_disk를 설정할 것. */
	swap_disk = NULL;
}

/* 파일 매핑을 초기화함. */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* 처리기를 설정함. */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* 스왑 디스크에서 내용을 읽어 페이지를 스왑 인함. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* 스왑 디스크에 내용을 기록하여 페이지를 스왑 아웃함. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* 익명 페이지를 정리함. PAGE 자체는 호출자가 해제함. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
