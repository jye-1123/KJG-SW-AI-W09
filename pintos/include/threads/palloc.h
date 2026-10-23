#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stdint.h>
#include <stddef.h>

/* 페이지 할당 방법을 지정하는 플래그. */
enum palloc_flags {
	PAL_ASSERT = 001,           /* 할당 실패 시 커널 패닉 발생. */
	PAL_ZERO = 002,             /* 페이지 내용을 0으로 초기화. */
	PAL_USER = 004              /* 사용자 풀의 페이지. */
};

/* 사용자 풀에 배정할 최대 페이지 수. */
extern size_t user_page_limit;

uint64_t palloc_init (void);
void *palloc_get_page (enum palloc_flags);
void *palloc_get_multiple (enum palloc_flags, size_t page_cnt);
void palloc_free_page (void *);
void palloc_free_multiple (void *, size_t page_cnt);

#endif /* threads/palloc.h */
