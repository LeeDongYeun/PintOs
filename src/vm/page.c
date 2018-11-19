#include "vm/page.h"
#include <stdio.h>
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/swap.h"


static unsigned
page_hash_func(const struct hash_elem *e, void *aux UNUSED){
	struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);

	return hash_int(pte->vaddr);
}

static bool
page_hash_less_func(const struct hash_elem *a,
					const struct hash_elem *b, void *aux UNUSED){
	struct page_table_entry *pte_a = hash_entry(a, struct page_table_entry, elem);
	struct page_table_entry *pte_b = hash_entry(b, struct page_table_entry, elem);

	if(pte_a->vaddr < pte_b->vaddr)
		return true;
	else 
		return false;
}

static bool
page_hash_destroy_func(struct hash_elem *e, void *aux UNUSED){
	ASSERT(e != NULL);

	struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);
	if(pte->frame != NULL)
		frame_free(pte->frame);
	pagedir_clear_page(thread_current()->pagedir, pte->vaddr);
	free(pte);
}

void
page_table_init(struct hash *pt){
	ASSERT(pt !=NULL);
	hash_init(pt, page_hash_func, page_hash_less_func, NULL);
}

void
page_table_destroy(struct hash *pt){
	ASSERT(pt != NULL);
	hash_destroy(pt, page_hash_destroy_func);
}

struct page_table_entry *
page_table_entry_alloc(void *vaddr, struct frame *frame, bool writable){
	struct page_table_entry *pte = malloc(sizeof(struct page_table_entry));
	if(pte == NULL){
		printf("page_table_entry_alloc failed\n");
		return false;
	}
	pte->type = PTE_FRAME;
	pte->vaddr = pg_round_down(vaddr);
	pte->frame = frame;
	pte->swap_table_index = -1;
	pte->writable = writable;

	return pte;
}

struct page_table_entry *
page_table_entry_file(void *vaddr, struct file *file, int offset, 
						int read_bytes, int zero_bytes, bool writable){
	struct page_table_entry *pte = malloc(sizeof(struct page_table_entry));
	if(pte == NULL){
		printf("page_table_entry_alloc failed\n");
		return false;
	}
	pte->type = PTE_FILE;
	pte->vaddr = pg_round_down(vaddr);
	pte->swap_table_index = -1;
	pte->writable = writable;

	pte->file = file;
	pte->offset = offset;
	pte->read_bytes = read_bytes;
	pte->zero_bytes = zero_bytes;

	return pte;
}

struct page_table_entry *
page_table_entry_mmap(void *vaddr, struct file *file, int offset, 
						int read_bytes, int zero_bytes, bool writable){
	struct page_table_entry *pte = malloc(sizeof(struct page_table_entry));
	if(pte == NULL){
		printf("page_table_entry_alloc failed\n");
		return false;
	}

	pte->type = PTE_MMAP;
	pte->vaddr = pg_round_down(vaddr);
	pte->swap_table_index = -1;
	pte->writable = writable;

	pte->file = file;
	pte->offset = offset;
	pte->read_bytes = read_bytes;
	pte->zero_bytes = zero_bytes;

	return pte;
}

void
page_table_add(struct page_table_entry *pte){
	//ASSERT(thread_current()->page_table);
	ASSERT(pte != NULL);

	//printf("page_table_added\n");

	hash_insert(&thread_current()->page_table, &pte->elem);
}

void
page_table_delete(struct page_table_entry *pte){
	printf("page_table_delete - pte->vaddr = %p\n", pte->vaddr);
	//ASSERT(thread_current()->page_table != NULL);

	hash_delete(&thread_current()->page_table, &pte->elem);
	if(pte->frame != NULL){
		printf("page_table_delete - frame is not null\n");
		frame_free(pte->frame);
	}
	//printf("page_table_delete - frame NULL\n");

	if(pte->swap_table_index != -1){
		swap_free(pte->swap_table_index);
	}

	//printf("page_table_delete - swap NULL\n");
	
	pagedir_clear_page(thread_current()->pagedir, pte->vaddr);
	//printf("page_table_delete - pagedir_clear_page\n");
	free(pte);
}

struct page_table_entry *
page_table_find(void *uaddr, struct thread *t){
	//ASSERT(&thread_current()->page_table != NULL);

	struct page_table_entry pte;
	struct hash_elem *e;

	pte.vaddr = pg_round_down(uaddr);
	e = hash_find(&t->page_table, &pte.elem);
	//printf("page_table_find - &thread_current()->tid = %d\n", &thread_current()->tid);

	if(e == NULL)
		return NULL;

	return hash_entry(e, struct page_table_entry, elem);
}

/*
bool
file_load(struct page_table_entry *pte){
	struct frame *frame;
	struct page_table_entry *victim_pte;
	bool success = false;

	frame = frame_alloc();
	if(frame == NULL){
		printf("adsf\n");
		frame = frame_replacement_select();
		victim_pte = page_table_find(frame->uaddr, frame->thread);
		victim_pte->swap_table_index = swap_add(frame->addr);
		pagedir_clear_page(frame->thread->pagedir, frame->uaddr);
		victim_pte->frame = NULL;
	}

	if (file_read_at(pte->file, frame->addr, pte->read_bytes, pte->offset) 
		!= (int) pte->read_bytes){
        frame_free(frame);
        return false; 
    }
    memset(frame->addr + pte->read_bytes, 0, pte->zero_bytes);

	return true;
}*/