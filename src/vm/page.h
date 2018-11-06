#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "frame.h"

struct page_table_entry{
	void *vaddr;
	struct frame *frame;
	struct hash_elem elem;
	int swap_table_index;
};

void page_table_init(struct hash *pt);
void page_table_destroy(struct hash *pt);
struct page_table_entry *page_table_entry_alloc(void *uaddr, struct frame *frame);
void page_table_add(struct page_table_entry *pte);
void page_table_delete(struct page_table_entry *pte);
struct page_table_entry *page_table_find(void *uaddr);

#endif