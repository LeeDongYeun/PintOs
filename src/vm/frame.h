#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/synch.h"

struct frame{
	struct list_elem elem;
	void *addr;
	bool accessable;
	void *uaddr;
};

struct list frame_table;

struct lock lock_frame;

void frame_table_init();
struct frame *frame_alloc();
void frame_set_accessable(struct frame *frame, bool boolean);
void frame_set_uaddr(struct frame *frame, void *uaddr);
void frame_add(struct frame *frame);
void frame_free(struct frame *frame);
struct frame *frame_find(void *addr);
struct frame *frame_replacement_select();

#endif 