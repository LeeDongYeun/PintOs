#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <stdint.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "lib/kernel/list.h"
#include "devices/input.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "userprog/exception.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

#ifndef MAX_STACK_SIZE
#define MAX_STACK_SIZE (1<<23)
#endif

typedef int pid_t;

static void syscall_handler (struct intr_frame *);

void halt(void);
void exit(int status);
pid_t exec(const char *cmd_line);
int wait(pid_t pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);

struct file *get_file(int fd);

static int 
get_user (const uint8_t *uaddr)
{
	if(uaddr >= PHYS_BASE)
  		return -1;
	int result;
	asm ("movl $1f, %0; movzbl %1, %0; 1:"
		: "=&a" (result) : "m" (*uaddr));
	return result;
}

int get_argv(char *ptr)
{
	unsigned temp1 = get_user(ptr);
	unsigned temp2 = get_user(ptr+1);
	unsigned temp3 = get_user(ptr+2);
	unsigned temp4 = get_user(ptr+3);

	
	if(temp1 == -1 || temp2 == -1 || temp3 == -1 || temp4 == -1)
	{	
		exit(-1);
	}
	else
	{	
		return temp1 + (temp2 << 8) + (temp3 << 16) + (temp4 << 24);
	}
}

void set_accessable(void *vaddr, bool boolean){
	printf("set_accessable\n");
	struct page_table_entry *pte = page_table_find(vaddr, thread_current());
	if(pte ==NULL){
		stack_growth(vaddr);
	}
	frame_set_accessable(pte->frame, boolean);
	printf("set_accessable done\n");
}

void set_accessable_buff(char *start, char *end, bool boolean){
	for(; start < end; start += PGSIZE)
			set_accessable(start, boolean);
}

/*
void
check_pointer(void *vaddr, void *esp){
	printf("check_pointer\n");
	struct page_table_entry *pte;
	bool success = false;
	
	if (!is_user_vaddr(vaddr) || vaddr < 0x08048000){
      exit(-1);
    }

    pte = page_table_find(vaddr);

  	if(pte == NULL){
    	if(vaddr < esp - 32){
      		exit(-1);
    	}

    	if(!(vaddr < PHYS_BASE && vaddr >= PHYS_BASE - MAX_STACK_SIZE)){
      		printf("fault2\n");
      		exit(-1);
    	}

    	success = stack_growth(vaddr);
    	if(!success){
      		exit(-1);
    	}
  }
}

void
check_string(const char *str, void *esp){
	for (; check_pointer((void *) str, esp), *str; str++); 
}

void
check_buffer(const char *buff, unsigned size, void *esp){
	while (size--)
    check_pointer((void *) (buff++), esp); 
}*/




/*
file descriptor로 syscall_init에서 초기화하고
open함수에서 file을 오픈할 때마다 1씩 증가한다  
*/
void
syscall_init (void) 
{
	lock_init(&lock_filesys);
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
	
}

static void
syscall_handler (struct intr_frame *f) 
{	
	int esp_val = (get_argv(f->esp));
	//check_pointer(f->esp, f->esp);

	//printf("handler esp = %p", f->esp);
	//printf("esp_val = %d\n", esp_val);
	
	//printf("syscall_handler started\n");

	switch(esp_val){
	  	case SYS_HALT:
	  		halt();
	  		break;

	  	case SYS_EXIT:
	  		exit((int)get_argv((int *)f->esp+1));
	  		break;

	  	case SYS_EXEC:
	  		//check_string((char*)get_argv((int *)f->esp+1), f->esp);
	  		f -> eax = (uint32_t) exec((char*)get_argv((int *)f->esp+1));
	  		break;

	  	case SYS_WAIT:
	  		f -> eax = (uint32_t) wait((int)get_argv((int *)f->esp+1));
	  		break;

	  	case SYS_CREATE:
	  		//check_string((char *)get_argv((int *)f->esp+1), f->esp);
	  		f -> eax = create((char *)get_argv((int *)f->esp+1), (unsigned)get_argv((int *)f->esp+2));
	  		break;

	  	case SYS_REMOVE:
	  		//check_string((char *)get_argv((int *)f->esp+1), f->esp);
	  		f -> eax = remove((char *)get_argv((int *)f->esp+1));
	  		break;

	  	case SYS_OPEN:
	  		//check_string((char *)get_argv((int *)f->esp+1), f->esp);
	  		f -> eax = open((char *)get_argv((int *)f->esp+1));
	  		break;

	  	case SYS_FILESIZE:

	  		f -> eax = filesize((int)get_argv((int *)f->esp+1));
	  		break;

	  	case SYS_READ:
	  		//check_buffer((void *)get_argv((int *)f->esp+2), (unsigned)get_argv((int *)f->esp+3), f->esp);
	  		f -> eax = read((int)get_argv((int *)f->esp+1), (void *)get_argv((int *)f->esp+2), (unsigned)get_argv((int *)f->esp+3));
	  		break;

	  	case SYS_WRITE:
	  		//check_buffer((void *)get_argv((int *)f->esp+2), (unsigned)get_argv((int *)f->esp+3), f->esp);
	  		f -> eax = write((int)get_argv((int *)f->esp+1), (void *)get_argv((int *)f->esp+2), (unsigned)get_argv((int *)f->esp+3));
	  		break;

	  	case SYS_SEEK:
	  		seek((int)get_argv((int *)f->esp+1), (unsigned)get_argv((int *)f->esp+2));
	  		break;

	  	case SYS_TELL:
	  		f -> eax = tell((int)get_argv((int *)f->esp+1));
	  		break;

	  	case SYS_CLOSE:
	  		close((int)get_argv((int *)f->esp+1));
	  		break;
	}
	
}


/*
현재 thread에 있는 file_list에서 fd가 같은 값을 찾은 후 file에 대한
포인터를 리턴한다. 만약 없다면 NULL을 리턴한다.
*/
struct file*
get_file(int fd){

	struct thread *curr = thread_current();
	struct list_elem *e;
	struct file_descriptor *file_descriptor;

	ASSERT (&curr->file_list != NULL);

	for(e = list_begin(&curr->file_list); e != list_end(&curr->file_list); e = list_next(e)){
		file_descriptor = list_entry(e, struct file_descriptor, elem);
		if(fd == file_descriptor->fd){
			return file_descriptor->file;
		}
	}

	return NULL;
}

void
halt(void){
	power_off();
}

void
exit(int status)
{	
	//printf("SYS_EXIT\n");
	struct thread *curr = thread_current ();
	curr->exit_status = status;
	printf("%s: exit(%d)\n", curr->name, status);

  	thread_exit();
}

pid_t
exec(const char *cmd_line){
	//printf("SYS_EXEC\n");
	int tid;
	struct thread *child_process;

	if(cmd_line == NULL)
		return -1;

	tid = process_execute(cmd_line);
	child_process = get_child_thread(tid);

	if(child_process != NULL){
		sema_down(&child_process->sema_load);
		if(!child_process->load_status)
			return -1;
	}
	
	return tid;
}


int
wait(pid_t pid)
{	
	//printf("SYS_WAIT\n");
	return process_wait(pid);
}

bool
create(const char *file, unsigned initial_size){
	//printf("SYS_CREATE\n");
	bool result;

	if(file==NULL)
		exit(-1);

	lock_acquire(&lock_filesys);
	result = filesys_create(file, initial_size);
	lock_release(&lock_filesys);

	return result;
}

bool
remove(const char *file){
	//printf("SYS_REMOVE\n");
	bool result;

	//check_pointer(file);
	lock_acquire(&lock_filesys);
	result = filesys_remove(file);
	lock_release(&lock_filesys);

	return result;
}

/*
파일을 연 후 각 파일마다 fd를 배정하여 파일을 연 thread의 file_list에
file_descriptor라는 구조체를 저장한다. 그 후 fd를 증가시켜 준다.
*/
int
open(const char *file){
	//printf("SYS_OPEN\n");
	if(file ==NULL)
		return -1;
	int result;

	struct thread *curr = thread_current();
	//check_pointer(file);
	lock_acquire(&lock_filesys);
	struct file *f = filesys_open(file);
	lock_release(&lock_filesys);
	
	if(!f){
		result = -1;
	}

	else{
		
		struct file_descriptor *file_descriptor = malloc(sizeof(struct file_descriptor));
		
		file_descriptor->file = f;
		file_descriptor->fd = curr->fd;
		list_push_back(&(curr->file_list), &(file_descriptor->elem));
		result = curr->fd;
		curr->fd++;
	}
	

	return result;
}

/*
파일이 존재한다면 길이를 리턴하고
존재하지 않는다면 -1을 리턴한다.
*/
int
filesize(int fd){
	//printf("SYS_FILESIZE\n");
	int result;

	lock_acquire(&lock_filesys);
	struct file * file = get_file(fd);

	if(!file){
		result = -1;
	}
	else{
		result = file_length(file);
	}
	lock_release(&lock_filesys);

	return result;
}


/*
fd 의 값이 0 이면 키보드로부터 버퍼에 값을 읽어오고,
아니면 fd에 맞는 file로부터 size만큼 값을 읽어온다.
*/
int
read(int fd, void *buffer, unsigned size){
	//printf("SYS_READ\n");
	int result;
	
	//set_accessable_buff(buffer, buffer + size, false);
	
	if(fd == STDIN_FILENO){
		unsigned i = 0;

		for(;i<size;i++)
			*((uint8_t *)buffer++) = input_getc();

		result = size;

	}
	else{
		lock_acquire(&lock_filesys);
		struct file *file = get_file(fd);

		if(!file){
			result = -1;
		}
		else{
			result = file_read(file, buffer, size);
		}
		lock_release(&lock_filesys);
	}

	//set_accessable_buff(buffer, buffer + size, true);
	return result;
}


int
write(int fd, const void *buffer, unsigned size){
	
	int result;

	//set_accessable_buff(buffer, buffer + size, false);
	
	if(fd == STDOUT_FILENO){
		putbuf(buffer, size);
		result = size;
	}
	else{
		
		struct file *file = get_file(fd);

		if(!file){
			result = 0;
		}
		else{
			lock_acquire(&lock_filesys);
			result = file_write(file, buffer, size);
			lock_release(&lock_filesys);
		}
		//printf("syscall_handler - SYS_WRITE -result = %d\n", result);
		
	}
	//set_accessable_buff(buffer, buffer + size, true);

	return result;
}

void
seek(int fd, unsigned position){
	//printf("SYS_SEEK\n");
	lock_acquire(&lock_filesys);
	struct file * file = get_file(fd);

	if(!file){
		return -1;
	}
	file_seek(file, position);
	lock_release(&lock_filesys);
}

unsigned
tell(int fd){
	//printf("SYS_TELL\n");
	off_t result;

	lock_acquire(&lock_filesys);
	struct file * file = get_file(fd);

	if(!file){
		result = -1;
	}
	else{
		result = file_tell(file);
	}
	lock_release(&lock_filesys);

	return result;
}

/*
현재 thread의 file_list에서 fd값이 일치하는 file을 찾은 후 
thread의 file_list에서 제거해주고, file 또한 닫는다.
*/
void
close(int fd){
	//printf("SYS_CLOSE\n");
	struct thread *curr = thread_current();
	struct list_elem *e;
	struct file_descriptor *file_descriptor;

	ASSERT (&curr->file_list != NULL);

	for(e = list_begin(&curr->file_list); e != list_end(&curr->file_list); e = list_next(e)){
		file_descriptor = list_entry(e, struct file_descriptor, elem);
		
		if(fd == file_descriptor->fd){
			list_remove(&file_descriptor->elem);

			lock_acquire(&lock_filesys);
			file_close(file_descriptor->file);
			lock_release(&lock_filesys);
			//free(file_descriptor);
			break;
		}
	}
}

