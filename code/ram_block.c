/* Disk on RAM Driver */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/errno.h>

#include "ram_device.h"

#define RB_FIRST_MINOR 0
#define RB_MINOR_CNT 4

static u_int rb_major = 0;


static struct rb_device
{
	unsigned int size;
	spinlock_t lock;
	struct request_queue *rb_queue;
	struct gendisk *rb_disk;
} rb_dev;

static int rb_open(struct block_device *bdev, fmode_t mode)
{
	unsigned unit = iminor(bdev->bd_inode);

	printk(KERN_INFO "rb: Device is opened\n");
	printk(KERN_INFO "rb: Inode number is %d\n", unit);

	if (unit > RB_MINOR_CNT)
		return -ENODEV;
	return 0;
}

static int rb_close(struct gendisk *disk, fmode_t mode)
{
	printk(KERN_INFO "rb: Device is closed\n");
	return 0;
}

static int rb_transfer(struct request *req)
{

	int dir = rq_data_dir(req);
	sector_t start_sector = blk_rq_pos(req);
	unsigned int sector_cnt = blk_rq_sectors(req);

	struct bio_vec bv;
	struct req_iterator iter;

	sector_t sector_offset;
	unsigned int sectors;
	u8 *buffer;

	int ret = 0;

	sector_offset = 0;
	rq_for_each_segment(bv, req, iter)
	{
		buffer = page_address(bv.bv_page) + bv.bv_offset;
		if (bv.bv_len % RB_SECTOR_SIZE != 0)
		{
			ret = -EIO;
		}
		sectors = bv.bv_len / RB_SECTOR_SIZE;
		printk(KERN_DEBUG "rb: Start sector: %lu; Sector Offset: %lld; Buffer: %p; Length: %d sectors\n",
			(unsigned long)start_sector,sector_offset, buffer, sectors);
		if (dir == WRITE) /* Write to the device */
		{
			ramdevice_write(start_sector + sector_offset, buffer, sectors);
		}
		else /* Read from the device */
		{
			ramdevice_read(start_sector + sector_offset, buffer, sectors);
		}
		sector_offset += sectors;
	}
	if (sector_offset != sector_cnt)
	{
		printk(KERN_ERR "rb: bio info doesn't match with the request info");
		ret = -EIO;
	}

	return ret;
}

static void rb_request(struct request_queue *q)
{
	struct request *req;
	int ret;


	while ((req = blk_fetch_request(q)) != NULL)
	{

		ret = rb_transfer(req);
		__blk_end_request_all(req, ret);
	}
}

//file operation struct on block device
static struct block_device_operations rb_fops =
{
	.owner = THIS_MODULE,
	.open = rb_open,
	.release = rb_close,
};

static int __init rb_init(void)
{
	int ret;

//allocate memory to the ram disk
	if ((ret = ramdevice_init()) < 0)
	{
		return ret;
	}
	rb_dev.size = ret;

//register the block device
	rb_major = register_blkdev(rb_major, "rb");
	if (rb_major <= 0)
	{
		printk(KERN_ERR "rb: Unable to get Major Number\n");
		ramdevice_cleanup();
		return -EBUSY;
	}

	spin_lock_init(&rb_dev.lock);
	rb_dev.rb_queue = blk_init_queue(rb_request, &rb_dev.lock);
	if (rb_dev.rb_queue == NULL)
	{
		printk(KERN_ERR "rb: blk_init_queue failure\n");
		unregister_blkdev(rb_major, "rb");
		ramdevice_cleanup();
		return -ENOMEM;
	}

	rb_dev.rb_disk = alloc_disk(RB_MINOR_CNT);
	if (!rb_dev.rb_disk)
	{
		printk(KERN_ERR "rb: alloc_disk failure\n");
		blk_cleanup_queue(rb_dev.rb_queue);
		unregister_blkdev(rb_major, "rb");
		ramdevice_cleanup();
		return -ENOMEM;
	}


	rb_dev.rb_disk->major = rb_major;
	rb_dev.rb_disk->first_minor = RB_FIRST_MINOR;
	rb_dev.rb_disk->fops = &rb_fops;
	rb_dev.rb_disk->private_data = &rb_dev;
	rb_dev.rb_disk->queue = rb_dev.rb_queue;
	
	sprintf(rb_dev.rb_disk->disk_name, "rb");
	set_capacity(rb_dev.rb_disk, rb_dev.size);

	
	add_disk(rb_dev.rb_disk); //make disk visible to computer

	printk(KERN_INFO "rb: Ram Block driver initialised (%d sectors; %d bytes)\n",
		rb_dev.size, rb_dev.size * RB_SECTOR_SIZE);

	return 0;
}

static void __exit rb_cleanup(void)
{
	del_gendisk(rb_dev.rb_disk);
	put_disk(rb_dev.rb_disk);
	blk_cleanup_queue(rb_dev.rb_queue);
	unregister_blkdev(rb_major, "rb");
	ramdevice_cleanup();
}

module_init(rb_init);
module_exit(rb_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anil Kumar Pugalia <email@sarika-pugs.com>");
MODULE_DESCRIPTION("Ram Block Driver");
MODULE_ALIAS_BLOCKDEV_MAJOR(rb_major);
