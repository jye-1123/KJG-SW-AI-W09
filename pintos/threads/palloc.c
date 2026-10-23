#include "threads/palloc.h"
#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "threads/init.h"
#include "threads/loader.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

/* 페이지 할당기. 페이지 크기 또는 그 배수 단위로 메모리를 할당함.
   더 작은 단위의 할당기는 malloc.h를 참고할 것.

   시스템 메모리는 커널 풀과 사용자 풀이라는 두 풀로 나뉨.
   사용자 풀은 사용자 가상 메모리 페이지를 위한 것이고,
   커널 풀은 그 밖의 모든 용도를 위한 것임.
   사용자 프로세스가 스왑을 빈번하게 수행하더라도
   커널 자체 작업에 필요한 메모리를 확보하려는 목적임.

   기본적으로 시스템 RAM의 절반씩을 두 풀에 배정함.
   커널 풀에는 필요 이상으로 많은 양이겠지만,
   동작을 보여 주는 용도로는 문제없음.
   */

/* 메모리 풀. */
struct pool {
	struct lock lock;               /* 상호 배제용 락. */
	struct bitmap *used_map;        /* 빈 페이지를 관리하는 비트맵. */
	uint8_t *base;                  /* 풀의 시작 주소. */
};

/* 커널 데이터용 풀과 사용자 페이지용 풀. */
static struct pool kernel_pool, user_pool;

/* 사용자 풀에 배정할 최대 페이지 수. */
size_t user_page_limit = SIZE_MAX;
static void
init_pool (struct pool *p, void **bm_base, uint64_t start, uint64_t end);

static bool page_from_pool (const struct pool *, void *page);

/* 멀티부트 정보. */
struct multiboot_info {
	uint32_t flags;
	uint32_t mem_low;
	uint32_t mem_high;
	uint32_t __unused[8];
	uint32_t mmap_len;
	uint32_t mmap_base;
};

/* E820 메모리 맵 항목. */
struct e820_entry {
	uint32_t size;
	uint32_t mem_lo;
	uint32_t mem_hi;
	uint32_t len_lo;
	uint32_t len_hi;
	uint32_t type;
};

/* ext_mem/base_mem의 주소 범위 정보를 나타냄. */
struct area {
	uint64_t start;
	uint64_t end;
	uint64_t size;
};

#define BASE_MEM_THRESHOLD 0x100000
#define USABLE 1
#define ACPI_RECLAIMABLE 3
#define APPEND_HILO(hi, lo) (((uint64_t) ((hi)) << 32) + (lo))

/* E820 항목을 순회하며 기본 메모리와 확장 메모리의 범위를 분석함. */
static void
resolve_area_info (struct area *base_mem, struct area *ext_mem) {
	struct multiboot_info *mb_info = ptov (MULTIBOOT_INFO);
	struct e820_entry *entries = ptov (mb_info->mmap_base);
	uint32_t i;

	for (i = 0; i < mb_info->mmap_len / sizeof (struct e820_entry); i++) {
		struct e820_entry *entry = &entries[i];
		if (entry->type == ACPI_RECLAIMABLE || entry->type == USABLE) {
			uint64_t start = APPEND_HILO (entry->mem_hi, entry->mem_lo);
			uint64_t size = APPEND_HILO (entry->len_hi, entry->len_lo);
			uint64_t end = start + size;
			printf("%llx ~ %llx %d\n", start, end, entry->type);

			struct area *area = start < BASE_MEM_THRESHOLD ? base_mem : ext_mem;

			// 이 영역에 속하는 첫 번째 항목.
			if (area->size == 0) {
				*area = (struct area) {
					.start = start,
					.end = end,
					.size = size,
				};
			} else {  // 그 밖의 경우.
				// 시작 주소를 확장함.
				if (area->start > start)
					area->start = start;
				// 끝 주소를 확장함.
				if (area->end < end)
					area->end = end;
				// 크기를 늘림.
				area->size += size;
			}
		}
	}
}

/* 풀을 구성함.
 * 코드 페이지를 포함한 모든 페이지를 이 할당기가 관리함.
 * 기본적으로 메모리의 절반씩을 커널과 사용자에게 배정함.
 * 가능한 많은 기본 메모리(base_mem)를 커널에 배정함.
 *
 */
static void
populate_pools (struct area *base_mem, struct area *ext_mem) {
	extern char _end;
	void *free_start = pg_round_up (&_end);

	uint64_t total_pages = (base_mem->size + ext_mem->size) / PGSIZE;
	uint64_t user_pages = total_pages / 2 > user_page_limit ?
		user_page_limit : total_pages / 2;
	uint64_t kern_pages = total_pages - user_pages;

	// 각 풀의 메모리 영역을 확보하기 위해 E820 맵을 분석함.
	enum { KERN_START, KERN, USER_START, USER } state = KERN_START;
	uint64_t rem = kern_pages;
	uint64_t region_start = 0, end = 0, start, size, size_in_pg;

	struct multiboot_info *mb_info = ptov (MULTIBOOT_INFO);
	struct e820_entry *entries = ptov (mb_info->mmap_base);

	uint32_t i;
	for (i = 0; i < mb_info->mmap_len / sizeof (struct e820_entry); i++) {
		struct e820_entry *entry = &entries[i];
		if (entry->type == ACPI_RECLAIMABLE || entry->type == USABLE) {
			start = (uint64_t) ptov (APPEND_HILO (entry->mem_hi, entry->mem_lo));
			size = APPEND_HILO (entry->len_hi, entry->len_lo);
			end = start + size;
			size_in_pg = size / PGSIZE;

			if (state == KERN_START) {
				region_start = start;
				state = KERN;
			}

			switch (state) {
				case KERN:
					if (rem > size_in_pg) {
						rem -= size_in_pg;
						break;
					}
					// 커널 풀을 구성함.
					init_pool (&kernel_pool,
							&free_start, region_start, start + rem * PGSIZE);
					// 다음 상태로 전환함.
					if (rem == size_in_pg) {
						rem = user_pages;
						state = USER_START;
					} else {
						region_start = start + rem * PGSIZE;
						rem = user_pages - size_in_pg + rem;
						state = USER;
					}
					break;
				case USER_START:
					region_start = start;
					state = USER;
					break;
				case USER:
					if (rem > size_in_pg) {
						rem -= size_in_pg;
						break;
					}
					ASSERT (rem == size);
					break;
				default:
					NOT_REACHED ();
			}
		}
	}

	// 사용자 풀을 구성함.
	init_pool(&user_pool, &free_start, region_start, end);

	// E820 항목을 순회하며 사용 가능한 메모리를 설정함.
	uint64_t usable_bound = (uint64_t) free_start;
	struct pool *pool;
	void *pool_end;
	size_t page_idx, page_cnt;

	for (i = 0; i < mb_info->mmap_len / sizeof (struct e820_entry); i++) {
		struct e820_entry *entry = &entries[i];
		if (entry->type == ACPI_RECLAIMABLE || entry->type == USABLE) {
			uint64_t start = (uint64_t)
				ptov (APPEND_HILO (entry->mem_hi, entry->mem_lo));
			uint64_t size = APPEND_HILO (entry->len_hi, entry->len_lo);
			uint64_t end = start + size;

			// TODO: 0x1000 ~ 0x200000 영역을 추가할 것. 지금은 고려하지 않음.
			// 이 영역의 모든 페이지는 사용할 수 없음.
			if (end < usable_bound)
				continue;

			start = (uint64_t)
				pg_round_up (start >= usable_bound ? start : usable_bound);
split:
			if (page_from_pool (&kernel_pool, (void *) start))
				pool = &kernel_pool;
			else if (page_from_pool (&user_pool, (void *) start))
				pool = &user_pool;
			else
				NOT_REACHED ();

			pool_end = pool->base + bitmap_size (pool->used_map) * PGSIZE;
			page_idx = pg_no (start) - pg_no (pool->base);
			if ((uint64_t) pool_end < end) {
				page_cnt = ((uint64_t) pool_end - start) / PGSIZE;
				bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);
				start = (uint64_t) pool_end;
				goto split;
			} else {
				page_cnt = ((uint64_t) end - start) / PGSIZE;
				bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);
			}
		}
	}
}

/* 페이지 할당기를 초기화하고 메모리 크기를 구함. */
uint64_t
palloc_init (void) {
  /* 링커가 기록한 커널의 끝 주소.
     kernel.lds.S를 참고할 것. */
	extern char _end;
	struct area base_mem = { .size = 0 };
	struct area ext_mem = { .size = 0 };

	resolve_area_info (&base_mem, &ext_mem);
	printf ("Pintos booting with: \n");
	printf ("\tbase_mem: 0x%llx ~ 0x%llx (Usable: %'llu kB)\n",
		  base_mem.start, base_mem.end, base_mem.size / 1024);
	printf ("\text_mem: 0x%llx ~ 0x%llx (Usable: %'llu kB)\n",
		  ext_mem.start, ext_mem.end, ext_mem.size / 1024);
	populate_pools (&base_mem, &ext_mem);
	return ext_mem.end;
}

/* 연속된 빈 페이지 PAGE_CNT개를 확보해 반환함.
   PAL_USER가 설정되면 사용자 풀에서, 아니면 커널 풀에서 확보함.
   FLAGS에 PAL_ZERO가 설정되면 페이지 내용을 0으로 채움.
   사용 가능한 페이지가 부족하면 널 포인터를 반환함.
   단, FLAGS에 PAL_ASSERT가 설정되어 있으면 커널 패닉을 일으킴.
   */
void *
palloc_get_multiple (enum palloc_flags flags, size_t page_cnt) {
	struct pool *pool = flags & PAL_USER ? &user_pool : &kernel_pool;

	lock_acquire (&pool->lock);
	size_t page_idx = bitmap_scan_and_flip (pool->used_map, 0, page_cnt, false);
	lock_release (&pool->lock);
	void *pages;

	if (page_idx != BITMAP_ERROR)
		pages = pool->base + PGSIZE * page_idx;
	else
		pages = NULL;

	if (pages) {
		if (flags & PAL_ZERO)
			memset (pages, 0, PGSIZE * page_cnt);
	} else {
		if (flags & PAL_ASSERT)
			PANIC ("palloc_get: out of pages");
	}

	return pages;
}

/* 빈 페이지 하나를 확보하고 그 커널 가상 주소를 반환함.
   PAL_USER가 설정되면 사용자 풀에서, 아니면 커널 풀에서 확보함.
   FLAGS에 PAL_ZERO가 설정되면 페이지 내용을 0으로 채움.
   사용 가능한 페이지가 없으면 널 포인터를 반환함.
   단, FLAGS에 PAL_ASSERT가 설정되어 있으면 커널 패닉을 일으킴.

   */
void *
palloc_get_page (enum palloc_flags flags) {
	return palloc_get_multiple (flags, 1);
}

/* PAGES에서 시작하는 PAGE_CNT개의 페이지를 해제함. */
void
palloc_free_multiple (void *pages, size_t page_cnt) {
	struct pool *pool;
	size_t page_idx;

	ASSERT (pg_ofs (pages) == 0);
	if (pages == NULL || page_cnt == 0)
		return;

	if (page_from_pool (&kernel_pool, pages))
		pool = &kernel_pool;
	else if (page_from_pool (&user_pool, pages))
		pool = &user_pool;
	else
		NOT_REACHED ();

	page_idx = pg_no (pages) - pg_no (pool->base);

#ifndef NDEBUG
	memset (pages, 0xcc, PGSIZE * page_cnt);
#endif
	ASSERT (bitmap_all (pool->used_map, page_idx, page_cnt));
	bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);
}

/* PAGE의 페이지를 해제함. */
void
palloc_free_page (void *page) {
	palloc_free_multiple (page, 1);
}

/* 풀 P를 START에서 시작하여 END에서 끝나도록 초기화함. */
static void
init_pool (struct pool *p, void **bm_base, uint64_t start, uint64_t end) {
  /* 풀의 시작 부분에 used_map을 둠.
     비트맵에 필요한 공간을 계산하고
     풀의 크기에서 그만큼을 뺌. */
	uint64_t pgcnt = (end - start) / PGSIZE;
	size_t bm_pages = DIV_ROUND_UP (bitmap_buf_size (pgcnt), PGSIZE) * PGSIZE;

	lock_init(&p->lock);
	p->used_map = bitmap_create_in_buf (pgcnt, *bm_base, bm_pages);
	p->base = (void *) start;

	// 모든 페이지를 사용할 수 없는 상태로 표시함.
	bitmap_set_all(p->used_map, true);

	*bm_base += bm_pages;
}

/* PAGE가 POOL에서 할당되었으면 true를,
   아니면 false를 반환함. */
static bool
page_from_pool (const struct pool *pool, void *page) {
	size_t page_no = pg_no (page);
	size_t start_page = pg_no (pool->base);
	size_t end_page = start_page + bitmap_size (pool->used_map);
	return page_no >= start_page && page_no < end_page;
}
