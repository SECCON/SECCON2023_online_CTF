#ifndef _KMEMO_PCI_H
#define _KMEMO_PCI_H

int reg_pcidrv(void);
void unreg_pcidrv(void);

int pcidrv_prepare_store(const uint32_t count, uint32_t *p_key);
int pcidrv_store_page(pgoff_t pgoff, void *page);
void pcidrv_cleanup_store(void);

int pcidrv_prepare_load(const uint32_t key, int *p_count);
int pcidrv_load_page(int index, pgoff_t *p_pgoff, void *page);
void pcidrv_cleanup_load(void);

#endif
