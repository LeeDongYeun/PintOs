#include <stdio.h>
#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"


void
frame_table_init(){
	list_init(&frame_table);
	lock_init(&lock_frame);
}

struct frame *
frame_alloc(){

	//lock_acquire(&lock_frame);

	void *page = palloc_get_page(PAL_USER);
	if(page == NULL){
		//printf("frame_alloc - palloc failed\n");
		return NULL;
	}
	struct frame *f = malloc(sizeof(struct frame));

	if(f== NULL){
		printf("frame_alloc - malloc failed\n");
		palloc_free_page(page);
		return NULL;
	}

	f->type = FRAME_STACK;
	f->addr = page;
	f->accessable = false;
	f->thread = thread_current();
	f->filename = 0;

	//printf("frame_alloc - &thread_current()->tid = %d\n", &thread_current()->tid);

	//lock_release(&lock_frame);
	//printf("frame_alloced\n");

	return f;
}

void
frame_set_accessable(struct frame *frame, bool boolean){
	lock_acquire(&lock_frame);
	frame->accessable = boolean;
	lock_release(&lock_frame);
}

void
frame_set_uaddr(struct frame * frame, void *uaddr){
  lock_acquire(&lock_frame);
  frame->uaddr = pg_round_down(uaddr);
  lock_release(&lock_frame);
}

void
frame_add(struct frame *frame){
	lock_acquire(&lock_frame);
	list_push_back(&frame_table, &frame->elem);
	lock_release(&lock_frame);
}

void
frame_free(struct frame *frame){
	lock_acquire(&lock_frame);
	list_remove(&frame->elem);
	palloc_free_page(frame->addr);
	free(frame);
	lock_release(&lock_frame);
}

struct frame *
frame_find(void *addr){
	ASSERT(!list_empty(&frame_table));

	lock_acquire(&lock_frame);

	struct list_elem *e;
	struct frame *f;

	for(e=list_begin(&frame_table); e!=list_end(&frame_table); e=list_next(e)){
		f = list_entry(e, struct frame, elem);
		if(f->addr == addr){
			lock_release(&lock_frame);
			return f;
		}
	}

	lock_release(&lock_frame);
	return NULL;
}

struct frame *
frame_replacement_select(){
	ASSERT(!list_empty(&frame_table));

	//printf("frame_replacement_select\n");

	struct list_elem *e = list_end(&frame_table);
	struct frame *f;

	while(true){
		f = list_entry(e, struct frame, elem);

		//printf("f->thread->tid = %d ", f->thread->tid);
		//printf("f->addr %x\n", f->addr);
		
		if(f->thread->pagedir != NULL){
			//printf("access bit = %d dirty bit %d\n", 
			//			pagedir_is_accessed(f->thread->pagedir, f->uaddr), 
			//			pagedir_is_dirty(f->thread->pagedir, f->uaddr));


			if(pagedir_is_accessed(f->thread->pagedir, f->uaddr)){

				pagedir_set_accessed(f->thread->pagedir, f->uaddr, false);
			}
			else if(f->accessable == true)
				//lock_release(&lock_frame);
				return f;
		}

		if(e == list_end(&frame_table)){
			e = list_begin(&frame_table);
		}
		else{
			e = list_next(e);
		}	
	}
}

struct frame *
frame_replacement_select_not_current(){
	struct list_elem *e;
	struct frame *f;

	for(e=list_begin(&frame_table); e!=list_end(&frame_table); e=list_next(e)){
    	f = list_entry(e, struct frame, elem);
    	//printf("f->thread->tid = %d ", f->thread->tid);

    	if(f->thread->pagedir != NULL){
    		if(pagedir_is_accessed(f->thread->pagedir, f->uaddr)){

				pagedir_set_accessed(f->thread->pagedir, f->uaddr, false);
			}
			else if(f->accessable == true && f->type == FRAME_STACK)
				//lock_release(&lock_frame);
				return f;
		}
	}

	return NULL;
}

struct frame *
frame_replacement_select_current(){
	struct list_elem *e;
	struct frame *f;

	for(e=list_begin(&frame_table); e!=list_end(&frame_table); e=list_next(e)){
    	f = list_entry(e, struct frame, elem);
    	//printf("f->thread->tid = %d ", f->thread->tid);

    	if(f->thread->pagedir != NULL){
    		if(pagedir_is_accessed(f->thread->pagedir, f->uaddr)){

				pagedir_set_accessed(f->thread->pagedir, f->uaddr, false);
			}
			else if(f->accessable == true)
				//lock_release(&lock_frame);
				return f;
		}
	}

	return NULL;
}

struct frame *
frame_replacement_select2(){
	struct frame *f;
	while(true){
		f= frame_replacement_select_not_current();
		if(f != NULL) return f;
		f = frame_replacement_select_current();
		if(f !=  NULL) return f;
	}
}
