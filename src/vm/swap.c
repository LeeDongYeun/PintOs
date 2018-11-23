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

	//lock_acquire(&lock_swap);
	swap_table_index = bitmap_scan_and_flip(swap_table, 0, DISK_SECTOR_NUMBER, false);

	if(swap_table_index == BITMAP_ERROR){
		printf("swap_table_index - BITMAP_ERROR\n");
		return -2;
	}

	for(i=0; i<DISK_SECTOR_NUMBER; i++){
		disk_write(swap_disk, swap_table_index + i, kaddr + i*DISK_SECTOR_SIZE);
	}
	//lock_release(&lock_swap);

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

  	swap_delete(frame->kaddr, pte->swap_table_index);
	pte->swap_table_index = -1;

	frame_to_table(frame, pte->vaddr);

	pte->frame = frame;
	success = install_page(pte->vaddr, frame->kaddr, pte->writable);
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
	
	lock_acquire(&lock_swap);

	victim_frame = frame_replacement_select();

	//if(victim_frame->filename)
	//	printf("swap_out victim_frame filename = %d\n", victim_frame->filename);

	if(victim_frame == NULL){
		printf("NULL FRAME\n");
		return false;
	} 

	victim_pte = page_table_find(victim_frame->vaddr, victim_frame->thread);
	if(victim_pte == NULL){
		printf("NULL PTE\n");
		return false;
	}

	swap_table_index = swap_add(victim_frame->kaddr);

	printf("swap out type = %d\n", (int)victim_pte->type);

	if(swap_table_index == -2){
		printf("swap_out BITMAP_ERROR\n");
		return false;
	}

	victim_pte->swap_table_index = swap_table_index;
	victim_pte->frame = NULL;
	printf("swaped out vaddr = %x kaddr = %x\n", victim_pte->vaddr, victim_frame->kaddr);
	pagedir_clear_page(victim_frame->thread->pagedir, victim_frame->vaddr);


	frame_free(victim_frame);

	lock_release(&lock_swap);

	//printf("swaped out vaddr = %x\n", victim_pte->vaddr);
	return true;
}