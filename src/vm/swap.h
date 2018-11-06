#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "threads/synch.h"
#include "devices/disk.h"
#include <bitmap.h>

#define DISK_SECTOR_NUMBER 8

struct bitmap *swap_table;
struct disk *swap_disk;

struct lock lock_swap;

void swap_init(void);
void swap_out(void *kaddr);
void swap_in(void *kaddr, size_t swap_table_index);
void swap_free(void *kaddr);

#endif