#ifndef VM_UNINIT_H
#define VM_UNINIT_H
#include "vm/vm.h"

struct page;
enum vm_type;

typedef bool vm_initializer (struct page *, void *aux);

/* 미초기화 페이지.
 * 지연 로딩(Lazy loading)을 구현하기 위한 유형임. */
struct uninit_page {
	/* 페이지의 내용을 초기화함. */
	vm_initializer *init;
	enum vm_type type;
	void *aux;
	/* struct page를 초기화하고 PA를 VA에 매핑함. */
	bool (*page_initializer) (struct page *, enum vm_type, void *kva);
};

void uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *kva));
#endif
