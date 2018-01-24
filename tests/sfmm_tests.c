#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "sfmm.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int find_list_index_from_size(int sz) {
	if (sz >= LIST_1_MIN && sz <= LIST_1_MAX) return 0;
	else if (sz >= LIST_2_MIN && sz <= LIST_2_MAX) return 1;
	else if (sz >= LIST_3_MIN && sz <= LIST_3_MAX) return 2;
	else return 3;
}


Test(sf_memsuite_student, malloc_4080, .init = sf_mem_init, .fini = sf_mem_fini){
	void*x = sf_malloc(4080);
	cr_assert_not_null(x);
	//there should be nothing
	free_list *fl = &seg_free_list[find_list_index_from_size(4080)];
	cr_assert_null(fl->head, "Unexpected block in the freeList");
}
Test(sf_memsuite_student, malloc_splinter, .init = sf_mem_init, .fini = sf_mem_fini){
	void* x = sf_malloc(3984); // 4000
	void* y = sf_malloc(96); // 112
	cr_assert_not_null(x, "x is NULL!");
	cr_assert_not_null(y, "y is NULL!");
	free_list *fl = &seg_free_list[find_list_index_from_size(4080)];
	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert(fl->head->header.block_size << 4 == 4080, "Free block size not what was expected!");
}
Test(sf_memsuite_student, free_coalescing, .init = sf_mem_init, .fini = sf_mem_fini){
	void *x = sf_malloc(40);
	void *y = sf_malloc(40);
	void *z = sf_malloc(40);
	cr_assert_not_null(y, "y is NULL!");
	sf_free(x);
	sf_free(z);

	free_list *fl = &seg_free_list[find_list_index_from_size(40)];
	free_list *fly = &seg_free_list[find_list_index_from_size(4010)];
	cr_assert_not_null(fl->head, "No block in expected free list!");
	// get coalesced with the block
	cr_assert_null(fl->head->next, "No block should be expected place");
	cr_assert_null(fly->head->next, "No block should be expected place");
}
Test(sf_memsuite_student, malloc_four_pages, .init = sf_mem_init, .fini = sf_mem_fini){
	void *x = sf_malloc(4096);

	cr_assert_not_null(x, "x is NULL!");
	free_list *fl = &seg_free_list[find_list_index_from_size(4010)];
	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert(fl->head->header.block_size << 4 == 4080, "Free block size not what was expected!");
}
Test(sf_memsuite_student, realloc_more_than_four_pages, .init = sf_mem_init, .fini = sf_mem_fini){
	sf_errno = 0;
	void *x = sf_malloc(50);
	x = sf_realloc(x, PAGE_SZ * 5);
	cr_assert_null(x, "x is not NULL!");
	cr_assert(sf_errno == EINVAL, "sf_errno is not EINVAL!");
}


