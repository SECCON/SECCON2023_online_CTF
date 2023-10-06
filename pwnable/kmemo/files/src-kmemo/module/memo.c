#include <linux/mm.h>
#include "kmemo/pci.h"
#include "memo.h"

struct memo *init_memo(void){
	struct memo *memo;

	if(!(memo = kmalloc(sizeof(struct memo), GFP_KERNEL)))
		return NULL;

	mutex_init(&memo->lock);
	memo->top = NULL;
	memo->count = 0;

	return memo;
}

void fini_memo(struct memo *memo){
	struct memo_page_table *top;

	if(!memo) return;

	mutex_lock(&memo->lock);

	if(!(top = memo->top))
		goto END;

	for(int i=0; i<PAGE_SIZE/sizeof(void*); i++){
		struct memo_page_table *med = top->entry[i];
		if(!med) continue;

		for(int j=0; j<PAGE_SIZE/sizeof(void*); j++)
			free_page((uintptr_t)med->entry[j]);
		free_page((uintptr_t)med);
	}
	free_page((uintptr_t)top);

END:
	mutex_unlock(&memo->lock);
	kfree(memo);
}

static void *__pgoff_to_memopage(struct memo *memo, const pgoff_t pgoff, const bool modable, void *new_page){
	void *ret = NULL;

	if(!memo) return NULL;

	mutex_lock(&memo->lock);

	struct memo_page_table **p_top = &memo->top;
	if(!*p_top && (!modable || !(*p_top = (void*)get_zeroed_page(GFP_KERNEL))))
		goto ERR;

	struct memo_page_table **p_med = (struct memo_page_table**)&(*p_top)->entry[(pgoff >> MEMOPAGE_TABLE_SHIFT) & ((1<<MEMOPAGE_TABLE_SHIFT)-1)];
	if(!*p_med && (!modable || !(*p_med = (void*)get_zeroed_page(GFP_KERNEL))))
		goto ERR;

	char **p_data = (char**)&(*p_med)->entry[pgoff & ((1<<MEMOPAGE_TABLE_SHIFT)-1)];
	if(modable && (!*p_data || new_page))
		*p_data = *p_data ? (free_page((uintptr_t)*p_data), new_page) : (memo->count++, (new_page ?: (void*)get_zeroed_page(GFP_KERNEL)));
	ret = *p_data;

ERR:
	mutex_unlock(&memo->lock);
	return ret;
}

const void *get_memo_ro(const struct memo *memo, const loff_t pos){
	return __pgoff_to_memopage((struct memo *)memo, pos >> PAGE_SHIFT, false, NULL);
}

void *get_memo_rw(struct memo *memo, const loff_t pos){
	return __pgoff_to_memopage(memo, pos >> PAGE_SHIFT, true, NULL);
}

#define CHECK_PERMISSION() (!current_uid().val || !current_euid().val || !current_gid().val || !current_egid().val)

int store_memo(struct memo *memo, uint32_t *p_key){
	struct memo_page_table *top;
	int ret;

	if(!CHECK_PERMISSION())
		return -EACCES;

	if(!memo)
		return -ENODATA;
	if(!(top = memo->top))
		return 0;

	mutex_lock(&memo->lock);
	if((ret = pcidrv_prepare_store(memo->count, p_key)) < 0)
		goto ERR;

	for(int i=0; i<PAGE_SIZE/sizeof(void*); i++){
		struct memo_page_table *med = top->entry[i];
		if(!med) continue;

		for(int j=0; j<PAGE_SIZE/sizeof(void*); j++){
			void *page = med->entry[j];
			if(!page) continue;

			if((ret = pcidrv_store_page((i << MEMOPAGE_TABLE_SHIFT) + j, page)) < 0)
				goto ERR;
		}
	}

	ret = 0;
ERR:
	pcidrv_cleanup_store();
	mutex_unlock(&memo->lock);
	return ret;
}

int load_memo(struct memo *memo, const uint32_t key){
	struct memo_page_table *top;
	int count;
	int ret;

	if(!CHECK_PERMISSION())
		return -EACCES;

	if(!memo)
		return -ENODATA;
	if((top = memo->top))
		return -EBUSY;

	if((ret = pcidrv_prepare_load(key, &count)) < 0)
		return ret;

	for(int i=0; i<count; i++){
		pgoff_t pgoff;
		void *page = (void*)get_zeroed_page(GFP_KERNEL);

		if((ret = pcidrv_load_page(i, &pgoff, page)) < 0)
			goto ERR;

		if(__pgoff_to_memopage(memo, pgoff, true, page) != page){
			ret = -ENOMEM;
			goto ERR;
		}

		continue;
ERR:
		free_page((uintptr_t)page);
		break;
	}

	pcidrv_cleanup_load();
	return ret;
}
