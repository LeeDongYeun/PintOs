#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/synch.h"
#include "threads/thread.h"

enum frame_type{
	FRAME_STACK,
	FRAME_FILE,
};

struct frame{
	enum frame_type type;
	struct list_elem elem;
	void *addr;
	bool accessable;
	void *uaddr;
	char *filename;

	struct thread *thread;
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
struct frame *frame_replacement_select_current();
struct frame *frame_replacement_select_not_current();
struct frame *frame_replacement_select2();
#endif 