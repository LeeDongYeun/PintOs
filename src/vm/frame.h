#include <list.h>

struct frame{
	struct list_elem elem;
	void *addr;
};

struct list frame_table;