#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/synch.h"
#include "threads/thread.h"


struct frame{
	struct list_elem elem;
	void *kaddr;
	void *vaddr;
	bool accessable;
	char *filename;

	struct thread *thread;
};

struct list frame_table;

struct lock lock_frame;

void frame_table_init();
struct frame *frame_alloc();
void frame_set_accessable(struct frame *frame, bool boolean);
void frame_set_vaddr(struct frame *frame, void *vaddr);
void frame_add(struct frame *frame);
void frame_free(struct frame *frame);
void frame_to_table(struct frame *frame, void *vaddr);
struct frame *frame_find(void *addr);
struct frame *frame_replacement_select();
struct frame *frame_replacement_select_current();
struct frame *frame_replacement_select_not_current();
struct frame *frame_replacement_select2();
#endif 