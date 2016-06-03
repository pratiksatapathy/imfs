#include <linux/string.h>

#include "partition.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#define SECTOR_SIZE 512
#define MBR_SIZE SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 // sizeof(PartEntry)
#define PARTITION_TABLE_SIZE 64 // sizeof(PartTable)
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55
#define BR_SIZE SECTOR_SIZE
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE_SIZE 2
#define BR_SIGNATURE 0xAA55

/*
0000 0200 8301 0107 0100 0000 0001 0000
0000 0200 8301 0107 0000 0001 0000 0100

0000 0200 8301 0107 ffff ffff ffff ffff

0000  ................
00001c0: 0200 0001 0107 0000 0001 0000 0000 0000 0100 0000 0000



0001  ................
00001d0: 0207 8302 010e 0101 0000 0001 0000 

0002  ................
00001e0: 020e 8303 0115 0102 0000 0001 0000 

0003  ................
00001f0: 0215 8303 041c 0103 0000 ff00 0000
*/
 //0000 0200 8308 0404 0100 0000 ff03 0000 0000
//00000000  00 01 01 00 83 fe 3f 7e  3f 00 00 00 80 21 1f 00

//partition table structure
typedef struct
{
	unsigned char boot_type; 
	unsigned char start_head;
	unsigned char start_sec:6;
	unsigned char start_cyl_hi:2;
	unsigned char start_cyl;
	unsigned char part_type;
	unsigned char end_head;
	unsigned char end_sec:6;
	unsigned char end_cyl_hi:2;
	unsigned char end_cyl;
	unsigned abs_start_sec;
	unsigned sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

//creating partiton entries
static PartTable def_part_table =
{
	{
		boot_type: 0x00,
		start_head: 0x00,
		start_sec: 0x02,
		start_cyl: 0x00,
		part_type: 0x83,
		end_head: 0x01,
		end_sec: 0x01,
		end_cyl: 0x07,
		abs_start_sec: 0x00000001,
		sec_in_part: 0x00000100
	},
{
		boot_type: 0x00,
		start_head: 0x01,
		start_sec: 0x02,
		start_cyl: 0x07,
		part_type: 0x83,
		end_head: 0x02,
		end_sec: 0x01,
		end_cyl: 0x0e,
		abs_start_sec: 0x00000101,
		sec_in_part: 0x00000100
	},
{
		boot_type: 0x00,
		start_head: 0x02,
		start_sec: 0x02,
		start_cyl: 0x0e,
		part_type: 0x83,
		end_head: 0x03,
		end_sec: 0x01,
		end_cyl: 0x15,
		abs_start_sec: 0x00000201,
		sec_in_part: 0x00000100
	},
{
		boot_type: 0x00,
		start_head: 0x03,
		start_sec: 0x02,
		start_cyl: 0x15,
		part_type: 0x83,
		end_head: 0x03,
		end_sec: 0x04,
		end_cyl: 0x1C,
		abs_start_sec: 0x00000301,
		sec_in_part: 0x00000100
	}
};

//copying partition table to the in memory disk
void copy_partition_data(u8 *disk)
{
	memset(disk, 0x0, MBR_SIZE);
	*(unsigned long *)(disk + MBR_DISK_SIGNATURE_OFFSET) = 0x1313;
	memcpy(disk + PARTITION_TABLE_OFFSET, &def_part_table, PARTITION_TABLE_SIZE);
	*(unsigned short *)(disk + MBR_SIGNATURE_OFFSET) = MBR_SIGNATURE;
	
}
