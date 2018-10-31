#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/malloc.h"


void
frame_table_init(){
	list_init(&frame_table);
}

struct frame *
frame_alloc(){

	void *page = palloc_get_page(PAL_USER);
	struct frame *f = malloc(sizeof(struct frame_table_elem));

	f->addr = page;
	f->accessable = true;

	return f;
}

void
frame_add(struct frame *frame){
	list_push_back(&frame_table, &frame->elem);
}

void
frame_free(struct frame *frame){
	list_remove(&frame->elem);
	palloc_free_page(frame->addr);
	free(frame);
}

struct frame *
frame_find(void *addr){
	ASSERT(!list_empty(&frame_table));

	struct list_elem *e;
	struct frame *f;

	for(e=list_begin(&frame_table); e!=list_end(&frame_table); e=list_next(e)){
		f = list_entry(e, struct frame, elem);
		if(f->addr == addr){
			return f;
		}
	}

	return NULL;
}

struct frame *
frame_replacement_select(){
	ASSERT(!list_empty(&frame_table));

	struct list_elem *e = list_begin(&frame_table);
	struct frame *f = list_entry(e, struct frame, elem);

	while(true){
		
	}
}