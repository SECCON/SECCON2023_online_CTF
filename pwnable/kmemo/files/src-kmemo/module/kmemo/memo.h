#ifndef _TMEMO_MEMO_H
#define _TMEMO_MEMO_H

#define MEMOPAGE_SIZE_MAX (1<<(PAGE_SHIFT+9+9))

struct memo *init_memo(void);
void fini_memo(struct memo *memo);

const void *get_memo_ro(const struct memo *memo, const loff_t pos);
void *get_memo_rw(struct memo *memo, const loff_t pos);

int store_memo(struct memo *memo, uint32_t *p_key);
int load_memo(struct memo *memo, const uint32_t key);

#endif
