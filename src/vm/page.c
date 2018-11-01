#include <stdio.h>
#include "page.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

static unsigned
page_hash_func(const struct hash_elem *e, void *aux UNUSED){
	struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);

	return hash_int(pte->vaddr);
}

static bool
page_hash_less_func(const struct hash_elem *a,
					const struct hash_elem *b, voud *aux UNUSED){
	struct page_table_entry *pte_a = hash_entry(a, struct page_table_entry, elem);
	struct page_table_entry *pte_b = hash_entry(b, struct page_table_entry, elem);

	if(pte_a->uaddr < pte_b)
		return true;
	else 
		return false;
}

static bool
page_hash_destroy_func(struct hash_elem *e, void *aux UNUSED){
	ASSERT(e != NULL);

	struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);
	frame_free(pte->frame);
	pagedir_clear_page(thread_current()->pagedir, pte->uaddr);
	free(pte);
}

void
page_table_init(struct hash *pt){
	hash_init(pt, page_hash_func, page_hash_less_func, NULL);
}

void
page_table_destroy(struct hash *pt){
	ASSERT(pt != NULL);
	hash_destroy(pt, page_hash_destroy_func);
}

struct page_table_entry *
page_table_entry_alloc(void *uaddr, struct frame *frame){
	struct page_table_entry *pte = malloc(sizeof(sturct page_table_entry));
	pte->uaddr = uaddr;
	pte->frame = frame;

	return pte;
}

void
page_table_add(struct page_table_entry *pte){
	ASSERT(thread_current()->page_table !=NULL);
	ASSERT(pte != NULL);

	hash_insert(thread_current()->page_table, &pte->elem);
}

void
page_table_delete(struct page_table_entry *pte){
	ASSERT(thread_current()->page_table != NULL);

	hash_delete(thread_current()->page_table, &pte->elem);
	frame_free(pte->frame);
	pagedir_clear_page(thread_current()->pagedir, pte->uaddr);
	free(pte);
}

struct page_table_entry *
page_table_find(void *uaddr){
	ASSERT(thread_current()->page_table != NULL);

	struct hash *page_table = thread_current()->page_table;
	struct page_table_entry *pte;
	struct hash_elem *e;

	pte.uaddr = pg_round_down(uaddr);
	e = hash_find(page_table, &pte.uaddr);

	if(e == NULL)
		return NULL;

	return hash_entry(e, struct page_table_entry, elem);
}