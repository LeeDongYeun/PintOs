#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <list.h>
#include "frame.h"

enum pte_type{
	PTE_FRAME,
	PTE_FILE,
	PTE_MMAP,
};

struct page_table_entry{
	enum pte_type type;
	void *vaddr;
	struct frame *frame;
	int swap_table_index;
	bool writable;
	
	struct file *file;
	int offset;
	int read_bytes;
	int zero_bytes;

	struct hash_elem elem;
	struct list_elem mmap_elem;
};



void page_table_init(struct hash *pt);
void page_table_destroy(struct hash *pt);
struct page_table_entry *page_table_entry_alloc(void *vaddr, struct frame *frame, bool writable);
struct page_table_entry *page_table_entry_file(void *vaddr, struct file *file, int offset, 
												int read_bytes, int zero_bytes, bool writable);
struct page_table_entry *page_table_entry_mmap(void *vaddr, struct file *file, int offset, 
												int read_bytes, int zero_bytes, bool writable);
void page_table_add(struct page_table_entry *pte);
void page_table_delete(struct page_table_entry *pte);
struct page_table_entry *page_table_find(void *uaddr, struct thread *t);
//bool file_load(struct page_table_entry *pte);
#endif