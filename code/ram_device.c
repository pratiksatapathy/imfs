#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include "ram_device.h"
#include "partition.h"

#define RB_DEVICE_SIZE 1024 
static u8 *dev_data;//the start pointer to the in memory disk

//actual memory allocation happens here
int ramdevice_init(void)
{
	dev_data = vmalloc(RB_DEVICE_SIZE * RB_SECTOR_SIZE);
	if (dev_data == NULL)
		return -ENOMEM;

	copy_partition_data(dev_data);//write partition table to disk
	return RB_DEVICE_SIZE;
}

void ramdevice_cleanup(void)
{
	vfree(dev_data);
}
//real memory copy happens here
void ramdevice_write(sector_t sector_off, u8 *buffer, unsigned int sectors)
{
printk(KERN_INFO "writing to disk\n");
	memcpy(dev_data + sector_off * RB_SECTOR_SIZE, buffer,
		sectors * RB_SECTOR_SIZE);
}
void ramdevice_read(sector_t sector_off, u8 *buffer, unsigned int sectors)
{
printk(KERN_INFO "reading from disk\n");
	memcpy(buffer, dev_data + sector_off * RB_SECTOR_SIZE,
		sectors * RB_SECTOR_SIZE);
}
