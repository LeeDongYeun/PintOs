#include <stdio.h>
#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "threads/palloc.h"
#include "userprog/process.h"


void
swap_init(){
	swap_disk = disk_get(1,1);
	swap_table = bitmap_create(disk_size(swap_disk));
	lock_init(&lock_swap);
}

void
swap_add(void *kaddr){
	int i;
	int swap_table_index;

	lock_acquire(&lock_swap);
	swap_table_index = bitmap_scan_and_flip(swap_table, 0, DISK_SECTOR_NUMBER, false);

	if(swap_table_index == BITMAP_ERROR){
		printf("swap_table_index - BITMAP_ERROR\n");
		return BITMAP_ERROR;
	}

	for(i=0; i<DISK_SECTOR_NUMBER; i++){
		disk_write(swap_disk, swap_table_index + i, kaddr + i*DISK_SECTOR_SIZE);
	}
	lock_release(&lock_swap);

	return swap_table_index;
}

void
swap_delete(void *kaddr, int swap_table_index){
	int i;

	lock_acquire(&lock_swap);
	for(i=0; i<DISK_SECTOR_NUMBER; i++){
		disk_read(swap_disk, swap_table_index + i, kaddr + i*DISK_SECTOR_SIZE);
	}

	bitmap_set_multiple(swap_table, swap_table_index, DISK_SECTOR_NUMBER, false);
	lock_release(&lock_swap);

	return -1;
}

void
swap_free(int swap_table_index){
	lock_acquire(&lock_swap);
	bitmap_set_multiple(swap_table, swap_table_index, DISK_SECTOR_NUMBER, false);
	lock_release(&lock_swap);
}

bool
swap_in(struct page_table_entry *pte){
	
	struct frame *frame;
	bool success = false;

	if(pte->swap_table_index = -1){
		return false;
	}

	frame = frame_alloc();
	if(frame == NULL){
		printf("you should make evict part\n");
		return false;
	}

	else{

		frame_add(frame);
		frame_set_accessable(frame, true);

		pte->frame = frame;
		success = install_page(pte->vaddr, frame->addr, pte->writable);
      	if(!success){
      		page_table_delete(pte);
      		return false;
      	}

      	swap_delete(frame->addr, pte->swap_table_index);
		pte->swap_table_index = -1;
	}

	return true;
}