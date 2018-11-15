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

	void *kpage = palloc_get_page(PAL_USER);
	if(kpage == NULL){
		//printf("frame_alloc - palloc failed\n");
		return NULL;
	}
	struct frame *f = malloc(sizeof(struct frame));

	if(f== NULL){
		printf("frame_alloc - malloc failed\n");
		palloc_free_page(kpage);
		return NULL;
	}

	f->kaddr = kpage;
	//f->accessable = false;
	f->thread = thread_current();

	//printf("frame_alloc - &thread_current()->tid = %d\n", &thread_current()->tid);

	//lock_release(&lock_frame);
	//printf("frame_alloced\n");

	return f;
}

void
frame_set_accessable(struct frame *frame, bool boolean){
	frame->accessable = boolean;
}

void
frame_set_vaddr(struct frame * frame, void *vaddr){
  frame->vaddr = pg_round_down(vaddr);
}

void
frame_add(struct frame *frame){
	list_push_back(&frame_table, &frame->elem);
}

void
frame_to_table(struct frame *frame, void *vaddr){
	lock_acquire(&lock_frame);
	frame_set_accessable(frame, true);
	frame_set_vaddr(frame, vaddr);
	frame_add(frame);
	lock_release(&lock_frame);
}

void
frame_free(struct frame *frame){
	lock_acquire(&lock_frame);
	list_remove(&frame->elem);
	palloc_free_page(frame->kaddr);
	free(frame);
	lock_release(&lock_frame);
}

struct frame *
frame_find(void *kaddr){
	ASSERT(!list_empty(&frame_table));

	lock_acquire(&lock_frame);

	struct list_elem *e;
	struct frame *f;

	for(e=list_begin(&frame_table); e!=list_end(&frame_table); e=list_next(e)){
		f = list_entry(e, struct frame, elem);
		if(f->kaddr == kaddr){
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
			//			pagedir_is_accessed(f->thread->pagedir, f->vaddr), 
			//			pagedir_is_dirty(f->thread->pagedir, f->vaddr));


			if(pagedir_is_accessed(f->thread->pagedir, f->vaddr)){

				pagedir_set_accessed(f->thread->pagedir, f->vaddr, false);
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
    		if(pagedir_is_accessed(f->thread->pagedir, f->vaddr)){

				pagedir_set_accessed(f->thread->pagedir, f->vaddr, false);
			}
			else if(f->accessable == true)
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
    		if(pagedir_is_accessed(f->thread->pagedir, f->vaddr)){

				pagedir_set_accessed(f->thread->pagedir, f->vaddr, false);
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
