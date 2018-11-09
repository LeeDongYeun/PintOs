#include <stdio.h>
#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"


void
swap_init(){
	swap_disk = disk_get(1,1);
	swap_table = bitmap_create(disk_size(swap_disk));
	lock_init(&lock_swap);
}

int
swap_add(void *kaddr){
	int i;
	int swap_table_index;

	lock_acquire(&lock_swap);
	swap_table_index = bitmap_scan_and_flip(swap_table, 0, DISK_SECTOR_NUMBER, false);

	if(swap_table_index == BITMAP_ERROR){
		printf("swap_table_index - BITMAP_ERROR\n");
		return -2;
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

	if(pte->swap_table_index == -1){
		//printf("swap_in - swap_table_index = -1\n");
		return false;
	}

	frame = frame_alloc();

	if(frame == NULL){
    	//printf("swap_in - frame_alloc failed\n");
    	if(swap_out()){
          frame = frame_alloc();
        }
        else{
          return false;
        }
  	}

  	swap_delete(frame->addr, pte->swap_table_index);
	pte->swap_table_index = -1;

	frame_add(frame);
	frame_set_accessable(frame, true);
	frame_set_uaddr(frame, pte->vaddr);

	pte->frame = frame;
	success = install_page(pte->vaddr, frame->addr, pte->writable);
      if(!success){
      	page_table_delete(pte);
      	return false;
      }
    //printf("swap_in - success = %d\n", success);
	
	return true;
}

bool
swap_out(){
	struct frame *victim_frame;
	struct page_table_entry *victim_pte;
	int swap_table_index;
	void *check_page;

	victim_frame = frame_replacement_select();

	if(victim_frame == NULL){
		printf("NULL FRAME\n");
		return false;
	} 

	//printf("victim frame uaddr = %p\n", victim_frame->uaddr);

	victim_pte = page_table_find(victim_frame->uaddr, victim_frame->thread);
	if(victim_pte == NULL){
		printf("NULL PTE\n");
		return false;
	}

	swap_table_index = swap_add(victim_frame->addr);

	//printf("swap_out - swap_table_index = %d\n", swap_table_index);

	if(swap_table_index == -2){
		printf("swap_out BITMAP_ERROR\n");
		return false;
	}

	victim_pte->swap_table_index = swap_table_index;
	victim_pte->frame = NULL;

	pagedir_clear_page(victim_frame->thread->pagedir, victim_frame->uaddr);

	frame_free(victim_frame);

	check_page = palloc_get_page(PAL_USER);
	if(check_page == NULL){
		//printf("swap_out - palloc_get_page failed\n");
		return false;
	}
	else{
		//printf("swap_out - success\n");
		palloc_free_page(check_page);
	}

	if(victim_pte->frame == NULL){
    	if(victim_pte->swap_table_index == -1){
      		printf("swap_out - error pte\n");
      		exit(-1);
    	}
  	}

	return true;
}