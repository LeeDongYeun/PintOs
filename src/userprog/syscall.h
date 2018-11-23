#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/kernel/list.h"

void syscall_init (void);

struct file_descriptor{
	struct file *file;
	int fd;
	struct list_elem elem;
};

struct mmap_file{
	int map_id;
	struct file *file;
	struct list pte_list;
	struct list_elem elem;

};

struct lock lock_filesys;

#endif /* userprog/syscall.h */
