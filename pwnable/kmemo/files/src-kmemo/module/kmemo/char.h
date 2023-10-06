#ifndef _KMEMO_CHAR_H
#define _KMEMO_CHAR_H

#include <linux/fs.h>

int reg_chrdev(void);
void unreg_chrdev(void);

#endif
