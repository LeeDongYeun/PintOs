#include <list.h>

struct frame{
	struct list_elem elem;
	void *addr;
	bool accessable;
};

struct list frame_table;