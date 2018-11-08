#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "threads/synch.h"
#include "vm/page.h"
#include "devices/disk.h"
#include <bitmap.h>

#define DISK_SECTOR_NUMBER 8

struct bitmap *swap_table;
struct disk *swap_disk;

struct lock lock_swap;

void swap_init(void);
int swap_add(void *kaddr);
void swap_delete(void *kaddr, int swap_table_index);
void swap_free(int swap_table_index);
bool swap_in(struct page_table_entry *pte);

#endif