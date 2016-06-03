#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include "imfs.h"

int main( void )
{
    int fd;
    //char buf[4]="abc";
	struct imfs_disk_super_block dsb;
	//dsb = malloc(sizeof())
	dsb.version = 0;
	dsb.magic = 0x00001313;
	dsb.block_size = BLOCK_SIZE;
	dsb.inodes_count = 1;
	dsb.free_blocks = BLOCK_SIZE - 3;
	dsb.next_free_block = 3;
    fd = open("/dev/rb1", O_RDWR);
    lseek(fd, 0, SEEK_SET);
    write(fd, &dsb, BLOCK_SIZE);
    
    struct imfs_disk_inode dinode;
    
    dinode.inode_no = 0;
    dinode.data_block_number = ROOTDIR;
    
    lseek(fd, BLOCK_SIZE, SEEK_SET);
    write(fd, &dinode,sizeof(struct imfs_disk_inode) );
    close(fd);
    return 0;
}
