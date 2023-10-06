#ifndef _CHAR_H
#define _CHAR_H

#include "kmemo/module.h"

#define DEVICE_NAME MODULE_NAME
#define CLASS_NAME  DEVICE_NAME

#define IOCTL_MAGIC 'S'
#define MEMO_STORE  _IOR(IOCTL_MAGIC, 0, uint32_t)
#define MEMO_LOAD   _IOW(IOCTL_MAGIC, 1, uint32_t)

struct chrdev_info {
	unsigned int major;
	struct cdev cdev;
	struct class *class;
};

#endif
