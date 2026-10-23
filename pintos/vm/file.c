/* file.c: 파일을 메모리에 매핑한 객체(mmap 객체)의 구현. */

#include "vm/vm.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* 파일 기반 가상 메모리 하위 시스템을 초기화함. */
void
vm_file_init (void) {
}

/* 파일 기반 페이지를 초기화함. */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* 처리기를 설정함. */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* 파일에서 내용을 읽어 페이지를 스왑 인함. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* 수정된 내용을 파일에 다시 기록하여 페이지를 스왑 아웃함. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* 파일 기반 페이지를 정리함. PAGE 자체는 호출자가 해제함. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* mmap을 수행함. */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
}

/* munmap을 수행함. */
void
do_munmap (void *addr) {
}
