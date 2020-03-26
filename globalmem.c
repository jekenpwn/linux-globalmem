/*
 * globalmem.c
 *
 *  Created on: Mar 24, 2020
 *      Author: jeken
 *
 *  Licensed under GPLv2 or later
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVNAME "globalmem"

#define GLOBALMEM_SIZE 0x1000
#define GLOBALMEM_MAJOR 230

#define GLOBALMEM_MAGIC 'g'
#define GLOBALMEM_CLEAR _IO(GLOBALMEM_MAGIC,0)

static int globalmem_major = GLOBALMEM_MAJOR;


struct globalmem_dev_t {

	struct cdev ccdev;
	unsigned char* mem[GLOBALMEM_SIZE];

};

struct globalmem_dev_t *globalmem_devp;


static int globalmem_open(struct inode *node, struct file *filp){

	filp->private_data = globalmem_devp;
	return 0;
}

static int globalmem_release(struct inode *node, struct file *filp){
	return 0;
}

static int globalmem_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos){

	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;

	struct globalmem_dev_t *dev = (struct globalmem_dev_t *)filp->private_data;

	if(p > GLOBALMEM_SIZE)
		return -ENOMEM;

	/* if the count of data more than remain buffer size, do not write all data */
	if ( count > GLOBALMEM_SIZE - p){
		count = GLOBALMEM_SIZE - p;
	}
	/* write the date to globalmem from user */
	if (copy_from_user(dev->mem + p, buf, count))
		return -EFAULT;
	else{
		/* the position of globalmem need to been added */
		*ppos += count;
		ret = count;
		printk(KERN_INFO "written %u bytes(s) from %lu\n", count, p);
	}

	return ret;

}

static int globalmem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos){

	unsigned long p = *ppos;
	unsigned int count = size;
    int ret;

	struct globalmem_dev_t *dev = (struct globalmem_dev_t *)filp->private_data;

	if(p > GLOBALMEM_SIZE )
		return -ENOMEM;

	/* Check the size of reading are not out of the size of buffer */
	if(count > (GLOBALMEM_SIZE - p))
		count = GLOBALMEM_SIZE - p;
	/* read the date from globalmem to user */
	if(copy_to_user(buf,dev->mem + p,count))
		return -EFAULT;
	else{
		*ppos += count;
		ret = count;
		printk(KERN_INFO "read %u bytes(s) from %lu\n", count, p);
	}

	return ret;
}

static int globalmem_llseek(struct file *filp, loff_t offset, int orig){

	int ret;

	switch(orig){
	case 0:

		if( offset < 0 || (unsigned int)offset > GLOBALMEM_SIZE )
			return -EINVAL;

		filp->f_pos = offset;
		ret = filp->f_pos;
		break;

	case 1:

		if( (filp->f_pos + offset) > GLOBALMEM_SIZE || (filp->f_pos + offset) < 0 )
			return -EINVAL;

		filp->f_pos += offset;
		ret = filp->f_pos;
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

static long globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){

	struct globalmem_dev_t *dev = filp->private_data;

	switch(cmd){

	case GLOBALMEM_CLEAR:
		memset(dev->mem,0,GLOBALMEM_SIZE);
		break;

	}
	return 0;

}

static const struct file_operations globalmem_filp_op = {
		.owner 			= THIS_MODULE,
		.llseek 		= globalmem_llseek,
		.read 			= globalmem_read,
		.write 			= globalmem_write,
		.unlocked_ioctl = globalmem_ioctl,
		.open 			= globalmem_open,
		.release 		= globalmem_release,
};



static void globalmem_setup_cdev(struct globalmem_dev_t *dev, int index){

	int err, devno = MKDEV(globalmem_major, index);

	cdev_init(&dev->ccdev, &globalmem_filp_op);

	dev->ccdev.owner = THIS_MODULE;

	cdev_add(&dev->ccdev,devno,1);

	if(err)
		printk(KERN_NOTICE "Errot %d adding globalmem%d",err, index);

}


static int __init globalmem_init(void){

	int ret;
	dev_t devno = MKDEV(globalmem_major,0);

	if(globalmem_major){
		ret = register_chrdev_region(devno, 1, DEVNAME);
	}else{
		ret = alloc_chrdev_region(&devno, 0, 1, DEVNAME);
		globalmem_major = MAJOR(devno);
	}

	if (ret < 0)
		return ret;

	globalmem_devp = kzalloc(sizeof(struct globalmem_dev_t),GFP_KERNEL);

	if(!globalmem_devp) {
		ret = -ENOMEM;
		goto fail_zalloc;
	}

	globalmem_setup_cdev(globalmem_devp, 0);

	return 0;

fail_zalloc:
	unregister_chrdev_region(devno, 1);
	return ret;

}

static int __exit globalmem_exit(void){
	/* Delect cdev at first */
	cdev_del(&globalmem_devp->ccdev);
	/* Free kernel memory */
	kfree(globalmem_devp);
	/* Unregister char device now */
	unregister_chrdev_region(MKDEV(globalmem_major,0), 1);

	return 0;

}

module_init(globalmem_init);
module_exit(globalmem_exit);


MODULE_AUTHOR("Xueqin ZHUANG <jekenzhuang@foxmail.com> ");
MODULE_LICENSE("GPL v2");



