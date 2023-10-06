#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include "kmemo/memo.h"
#include "char.h"

static struct chrdev_info cinfo = {};
static struct file_operations memo_chrdev_fops;

int reg_chrdev(void){
	dev_t dev;

	if(alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME))
		return -EBUSY;
	cinfo.major = MAJOR(dev);

	cdev_init(&cinfo.cdev, &memo_chrdev_fops);
	cinfo.cdev.owner = THIS_MODULE;

	if(cdev_add(&cinfo.cdev, dev, 1))
		goto ERR_CDEV_ADD;

	cinfo.class = class_create(THIS_MODULE, CLASS_NAME);
	if(IS_ERR(cinfo.class))
		goto ERR_CLASS_CREATE;

	device_create(cinfo.class, NULL, MKDEV(cinfo.major, 0), NULL, DEVICE_NAME);

	return 0;

ERR_CLASS_CREATE:
	cdev_del(&cinfo.cdev);
ERR_CDEV_ADD:
	unregister_chrdev_region(dev, 1);
	return -EBUSY;
}

void unreg_chrdev(void){
	device_destroy(cinfo.class, MKDEV(cinfo.major, 0));
	class_destroy(cinfo.class);

	cdev_del(&cinfo.cdev);
	unregister_chrdev_region(MKDEV(cinfo.major, 0), 1);
}

static int chrdev_open(struct inode *inode, struct file *filp){
	struct memo *memo;

	if(!(memo = init_memo()))
		return -EAGAIN;

	filp->private_data = (void*)memo;

	return 0;
}

static int chrdev_close(struct inode *inode, struct file *filp){
	fini_memo(filp->private_data);
	filp->private_data = NULL;

	return 0;
}

static ssize_t chrdev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
	const struct memo *memo = filp->private_data;
	size_t remain;

	if(!memo)
		return -EIO;

	mutex_lock(&filp->f_pos_lock);
	for(remain = count; remain > 0; ){
		const loff_t poff = *f_pos % 0x1000;
		const size_t len = poff + remain > 0x1000 ? 0x1000 - poff : remain;

		const char *data = get_memo_ro(memo, *f_pos);
		if(!data || copy_to_user(buf, data + poff, len))
			if(clear_user(buf, len))
				goto ERR;

		*f_pos += len;
		buf += len;
		remain -= len;
	}

ERR:
	mutex_unlock(&filp->f_pos_lock);
	return count-remain;
}

static ssize_t chrdev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
	struct memo *memo = filp->private_data;
	size_t remain;

	if(!memo)
		return -EIO;

	mutex_lock(&filp->f_pos_lock);
	for(remain = count; remain > 0; ){
		const loff_t poff = *f_pos % 0x1000;
		const size_t len = poff + remain > 0x1000 ? 0x1000 - poff : remain;

		char *data = get_memo_rw(memo, *f_pos);

		if(!data || copy_from_user(data + poff, buf, len))
			goto ERR;

		*f_pos += len;
		buf += len;
		remain -= len;
	}

ERR:
	mutex_unlock(&filp->f_pos_lock);
	return count-remain;
}

static loff_t chrdev_seek(struct file *filp, loff_t offset, int whence){
	loff_t new_pos;

	switch(whence){
		case SEEK_SET:
			new_pos = offset;
			break;
		case SEEK_CUR:
			new_pos = filp->f_pos + offset;
			break;
		default:
			return -ENOSYS;
	}

	if(new_pos < 0 || new_pos >= MEMOPAGE_SIZE_MAX)
		return -EINVAL;

	return filp->f_pos = new_pos;
}

static long chrdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	struct memo *memo = filp->private_data;
	long ret = -EINVAL;

	switch(cmd){
		case MEMO_STORE:
			{
			uint32_t key;

			if(!arg) goto ERR;
			if((ret = store_memo(memo, &key)) < 0)
				goto ERR;
			ret = put_user(key, (uint32_t __user *)arg);
			}
			break;
		case MEMO_LOAD:
			ret = load_memo(memo, (uint32_t)arg);
			break;
	}

ERR:
	return ret;
}

static vm_fault_t mmap_fault(struct vm_fault *vmf){
	struct memo *memo = vmf->vma->vm_private_data;
	if(!memo)
		return VM_FAULT_OOM;

	char *data = get_memo_rw(memo, vmf->pgoff << PAGE_SHIFT);
	if(!data)
		return VM_FAULT_OOM;

	vmf->page = virt_to_page(data);

	return 0;
}

struct vm_operations_struct mmap_vm_ops = {
	.fault 	= mmap_fault,
};

static int chrdev_mmap(struct file *filp, struct vm_area_struct *vma){
	if((vma->vm_pgoff << PAGE_SHIFT) + vma->vm_end - vma->vm_start  > MEMOPAGE_SIZE_MAX)
		return -ENOMEM;

	vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = filp->private_data;
	vma->vm_file = filp;

	return 0;
}

static struct file_operations memo_chrdev_fops = {
	.owner          = THIS_MODULE,
	.open           = chrdev_open,
	.release        = chrdev_close,
	.read           = chrdev_read,
	.write          = chrdev_write,
	.llseek         = chrdev_seek,
	.unlocked_ioctl = chrdev_ioctl,
	.mmap           = chrdev_mmap,
};

