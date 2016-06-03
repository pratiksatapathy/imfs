#define BLOCK_SIZE 4096
#define SB_BLOCK 0
#define INODE_STORE 1
#define ROOTDIR 2
#define MAX_INODES 29

//max_inodes = BLOCK_SIZE/INODE_SIZE. It comes aroound 146. We roundoff to 140

struct imfs_disk_super_block {
	
	uint64_t version;
	uint64_t magic;
	uint64_t block_size;

	uint64_t inodes_count;
	
	uint64_t free_blocks;
	uint64_t next_free_block;

	char padding[4048]; //512 - 5*8 ,i.e., sector size - 8*fields

};

struct imfs_disk_inode {

	mode_t mode;
	uint64_t inode_no;
	uint64_t data_block_number;

	union {
		uint64_t file_size;
		uint64_t dir_children_count;
	};

};/*size 28 bytes*/

struct imfs_dir_record {

	char filename[100];
	uint64_t inode_no;

};
