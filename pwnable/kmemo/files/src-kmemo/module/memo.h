#ifndef _MEMO_H
#define _MEMO_H

#include <linux/mutex.h>

#define MEMOPAGE_TABLE_SHIFT (9)

struct memo_page_table {
	void* entry[PAGE_SIZE/sizeof(void*)];
};

struct memo {
	struct memo_page_table *top;
	uint32_t count;
	struct mutex lock;
};

#endif
