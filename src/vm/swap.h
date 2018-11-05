#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "threads/synch.h"
#include "devices/disk.h"
#include <bitmap.h>

void swap_init(void);
void swap_in(void *addr);
void swap_out(void *addr);