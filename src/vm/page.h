#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "frame.h"

struct page_table_entry{
	void *vaddr;
	struct frame *frame;
	struct hash_elem elem;
};

#endif