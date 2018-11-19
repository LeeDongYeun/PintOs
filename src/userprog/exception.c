#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"

#ifdef VM
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#endif

#define MAX_STACK_SIZE (1<<23)

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);
bool page_fault_process(void *fault_addr);
bool lazy_load_file(struct page_table_entry *pte);
bool lazy_load_mmap(struct page_table_entry *pte);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;
  
#ifdef VM
  bool success = false;
  struct thread *curr = thread_current();
 
  if(user){
    curr->esp = f->esp;
  }

  //printf("page faulted. fault_addr = %p esp = %p tid = %d\n", fault_addr, curr->esp, curr->tid);

  /*읽기 전용 페이지에 쓰기를 시도할 경우*/
  if(!not_present){
    //printf("write to read only page\n");
    exit(-1);
  }

  /*페이지가 커널 가상 메모리에 있는 경우*/
  if(is_kernel_vaddr(fault_addr) || fault_addr <0x08048000){
    //printf("is kernel vaddr\n ");
    exit(-1);
  }

  success = page_fault_process(fault_addr);
  if(!success)
    exit(-1);

#else
  f->eip = f->eax;
  f->eax = 0xffffffff;
  
  exit(-1);
  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */
  // printf ("Page fault at %p: %s error %s page in %s context.\n",
          // fault_addr,
          // not_present ? "not present" : "rights violation",
          // write ? "writing" : "reading",
          // user ? "user" : "kernel");
  // kill (f);
#endif
}

bool 
page_fault_process(void *fault_addr){
  bool success = false;
  struct frame *frame;
  struct page_table_entry *pte;
  struct thread *curr = thread_current();
  void *check_page;

  pte = page_table_find(fault_addr, curr);
  //printf("adfasd\n");

  if(pte == NULL){
   //printf("you should make stack expand\n");
    if(fault_addr < curr->esp - 32){
      //printf("fault_addr = %x curr->esp - 32 = %x\n", fault_addr, curr->esp-32);
      //printf("fault1\n");
      exit(-1);
    }

    if(!(fault_addr < PHYS_BASE && fault_addr >= PHYS_BASE - MAX_STACK_SIZE)){
      //printf("fault2\n");
      exit(-1);
    }

    success = stack_growth(fault_addr);
    if(!success){
      exit(-1);
    }
  }

  else{
    if(pte->type == PTE_FRAME){
      success = swap_in(pte);
    }
    
    else if(pte->type == PTE_FILE){
      success = lazy_load_file(pte);
    }

    else if(pte->type == PTE_MMAP){
      success = lazy_load_mmap(pte);
    }
      

    if(!success){
      exit(-1);
    }
  }

  return success;
}

bool
lazy_load_file(struct page_table_entry *pte){
  //printf("lazy_load_file vaddr = %x\n", pte->vaddr);
  //printf("lazy load file addr = %x\n", pte->file);
  struct frame *frame;

  frame = frame_alloc();

  if(frame == NULL){
    if(swap_out()){
      frame = frame_alloc();
    }
    else{
      return false;
    }
  }

  if (file_read_at(pte->file, frame->kaddr, pte->read_bytes, pte->offset) 
          != (int) pte->read_bytes){
        printf("file didn't read\n");
        frame_free(frame);
        return false; 
      }

  memset(frame->kaddr + pte->read_bytes, 0, pte->zero_bytes);

  if (!install_page (pte->vaddr, frame->kaddr, pte->writable)){
      free(frame);
      return false; 
  }
  pte->frame =frame;
  frame_to_table(frame, pte->vaddr);

  //printf("page_fault - file read to frame\n");

  return true;
}

bool
lazy_load_mmap(struct page_table_entry *pte){
  struct frame *frame;

  frame = frame_alloc();

  if(frame == NULL){
    if(swap_out()){
      frame = frame_alloc();
    }
    else{
      return false;
    }
  }

  //printf("lazy_load_mmap - vaddr = %x kaddr = %x read_bytes = %d offset = %d\n", pte->vaddr, frame->kaddr, pte->read_bytes, pte->offset);

  if (file_read_at(pte->file, frame->kaddr, pte->read_bytes, pte->offset) 
          != (int) pte->read_bytes){
        printf("mmap didn't read\n");
        //frame_free(frame);
        return false; 
      }

  memset(frame->kaddr + pte->read_bytes, 0, pte->zero_bytes);

  if (!install_page (pte->vaddr, frame->kaddr, pte->writable)){
      //free(frame);
      printf("lazy_load_mmap - install_page failed\n");
      return false; 
  }
  //pagedir_set_dirty(thread_current()->pagedir, pte->vaddr, false);

  pte->frame = frame;
  frame_to_table(frame, pte->vaddr);
  frame_set_accessable(frame, false);

  return true;
}
