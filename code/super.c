#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "imfs.h"

ssize_t imfs_write(struct file * , const char __user *, size_t ,loff_t *);
ssize_t imfs_read(struct file * , char __user *, size_t ,loff_t *);

//imfs file operation struct
const struct file_operations imfs_file_operations = {
	.read = imfs_read,
	.write = imfs_write,
};
//imfs function prototypes
struct dentry *imfs_lookup(struct inode*,struct dentry*, unsigned int);
int imfs_mkdir(struct inode *,struct dentry *,umode_t);
int imfs_create(struct inode *,struct dentry *, umode_t , bool );
static int imfs_iterate(struct file *, struct dir_context *);

//imfs inode operation struct
const struct inode_operations imfs_inode_ops = {
	.lookup     = imfs_lookup,
	.mkdir      = imfs_mkdir,
	.create     = imfs_create
};
//imfs directory operation struct
const struct file_operations imfs_dir_operations = {

	.iterate = imfs_iterate,

};
//function implementation for "ls" command operation
static int imfs_iterate(struct file *filp, struct dir_context *ctx)
{
	loff_t pos;
	struct inode *inode;
	struct super_block *sb;
	struct buffer_head *bh;
	struct imfs_disk_inode *imfs_inode;
	struct imfs_dir_record *record;
	int i;

	pos = ctx->pos;
	inode = filp->f_path.dentry->d_inode;
	sb = inode->i_sb;

	if (pos) { //check this,

		return 0;
	}

	imfs_inode = (struct imfs_disk_inode *)inode->i_private;

	if (unlikely(!S_ISDIR(imfs_inode->mode))) {//not a directory
		
		return -ENOTDIR;
	}

	bh = sb_bread(sb, imfs_inode->data_block_number);

	record = (struct imfs_dir_record *)bh->b_data;
	for (i = 0; i < imfs_inode->dir_children_count; i++) {
		dir_emit(ctx, record->filename, 255,record->inode_no, DT_UNKNOWN);
		ctx->pos += sizeof(struct imfs_dir_record);
		pos += sizeof(struct imfs_dir_record);
		record++;
	}
	brelse(bh);

	return 0;
}
//get the imfs inode structure from the metadata block
static struct inode *imfs_get_inode(struct super_block *sb, int ino)
{
	printk(KERN_INFO "In the imfs_get_inode.\n");
	struct inode *inode;
	struct imfs_disk_inode *imfs_inode, *iter;
	//search inode in ramdisk
	struct buffer_head *bh;
	bh = sb_bread(sb, INODE_STORE);
	iter = (struct imfs_disk_inode *)bh->b_data;
	iter = iter + ino;
	imfs_inode = iter;
	
	inode = new_inode(sb);
	inode->i_ino = ino;
	inode->i_sb = sb;
	inode->i_op = &imfs_inode_ops;

	if (S_ISDIR(imfs_inode->mode))
		inode->i_fop = &imfs_dir_operations;
	else if (S_ISREG(imfs_inode->mode))
		inode->i_fop = &imfs_file_operations;
	else
		printk(KERN_ERR "Unknown inode type. Neither a directory nor a file");

	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_private = imfs_inode;

	return inode;
}

int imfs_inode_save(struct super_block *sb, struct imfs_disk_inode *imfs_inode)
{
	struct imfs_disk_inode *inode_iterator;
	struct buffer_head *bh;

	bh = sb_bread(sb, INODE_STORE);

	inode_iterator = (struct imfs_disk_inode *)bh->b_data + imfs_inode->inode_no;

	if (likely(inode_iterator)) {
		memcpy(inode_iterator, imfs_inode, sizeof(*inode_iterator));
		printk(KERN_INFO "The inode updated\n");

		mark_buffer_dirty(bh);
	} else {
		printk(KERN_ERR
		       "The new filesize could not be stored to the inode.");
		return -EIO;
	}

	brelse(bh);

	return 0;
}

ssize_t imfs_write(struct file * filp, const char __user * buf, size_t len,loff_t * ppos)
{
	printk(KERN_ERR "write size %ld \n",len);
	
	struct inode *inode;
	struct imfs_disk_inode *imfs_inode;
	struct buffer_head *bh;
	struct super_block *sb;
	struct imfs_disk_super_block *imfs_sb;
	size_t safe_len;
	char *buffer;
	int retval;

	sb = filp->f_path.dentry->d_inode->i_sb;
	imfs_sb = sb->s_fs_info;
	
	inode = filp->f_path.dentry->d_inode;
	imfs_inode = inode->i_private;

	bh = sb_bread(filp->f_path.dentry->d_inode->i_sb,imfs_inode->data_block_number);

	if (!bh) {
		printk(KERN_ERR "Reading the block number [%llu] failed.",imfs_inode->data_block_number);
		return 0;
	}
	buffer = (char *)bh->b_data;

	/* Move the pointer until the required byte offset */
	buffer += *ppos;
	//using a safelength write capped by block number as we do not support file spanning multi blocks

	safe_len = min((size_t)4096, len);
	printk(KERN_ERR "write size %ld \n",safe_len);

	if (copy_from_user(buffer, buf, safe_len)) {
		brelse(bh);
		printk(KERN_ERR "Error copying file contents from the userspace buffer to the kernel space\n");
		return -EFAULT;
	}
	*ppos += len;

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	imfs_inode->file_size = safe_len;
	retval = imfs_inode_save(sb, imfs_inode);
	if (retval) {
		len = retval;
	}

	return len;
}
//getting data from datablock of a particular inode
ssize_t imfs_read(struct file * filp, char __user * buf, size_t len,loff_t * ppos)
{
	printk(KERN_ERR "readsize %ld \n",len);

	struct imfs_disk_inode *inode = filp->f_path.dentry->d_inode->i_private;
	struct buffer_head *bh;

	char *buffer;
	int nbytes;

	if (*ppos >= inode->file_size) {
		// stop read request with offset beyond the filesize 
		return 0;
	}

	bh = sb_bread(filp->f_path.dentry->d_inode->i_sb,inode->data_block_number);

	if (!bh) {
		printk(KERN_ERR "Reading failed : [%llu]",
		       inode->data_block_number);
		return 0;
	}

	buffer = (char *)bh->b_data;
	nbytes = min((size_t) inode->file_size, len);

	if (copy_to_user(buf, buffer, nbytes)) {
		brelse(bh);
		return -EFAULT;
	}

	brelse(bh);
	*ppos += nbytes;
	return nbytes;
}


struct dentry *imfs_lookup(struct inode *parent_inode,struct dentry *child_dentry, unsigned int flags)
{
	printk(KERN_INFO "IN the imfs_lookup.\n");
	struct imfs_disk_inode *parent = (struct imfs_disk_inode *)parent_inode->i_private;
	struct super_block *sb = parent_inode->i_sb;
	struct buffer_head *bh;
	struct imfs_dir_record *record;
	int i;

	bh = sb_bread(sb, parent->data_block_number);
//check this very imp   imfs_dir_record
	record = (struct imfs_dir_record *)bh->b_data;
	//we loop through the name to inode mapping and match file name
	for (i = 0; i < parent->dir_children_count; i++) {

		if (!strcmp(record->filename, child_dentry->d_name.name)) {
			
			struct inode *inode = imfs_get_inode(sb, record->inode_no);
			inode_init_owner(inode, parent_inode, ((struct imfs_disk_inode *)inode->i_private)->mode);
			d_add(child_dentry, inode);
			return NULL;
		}
		record++;
	}

	printk(KERN_ERR "No inode found for the filename [%s]\n",child_dentry->d_name.name);

	return NULL;
}

int imfs_mkdir(struct inode *i,struct dentry *d,umode_t mode)
{
	
	printk(KERN_INFO "Mkdir was executed.\n");
	bool flag;
	imfs_create(i,d,mode | S_IFDIR,flag);
	
	return 0;

}

int imfs_create(struct inode *dir,struct dentry *dentry, umode_t mode, bool flag)
{
	printk(KERN_INFO "Create was executed.\n");
	
	struct inode *inode;
	struct imfs_disk_inode *imfs_inode;
	struct super_block *sb;
	struct imfs_disk_inode *parent_dir_inode;
	struct buffer_head *bh,*bh1;
	struct imfs_dir_record *dir_contents_datablock,*new_record;
	struct imfs_disk_super_block *dsb;
	uint64_t count;
	int ret;

	
	sb = dir->i_sb;
	dsb = sb->s_fs_info;
	count = dsb->inodes_count;
	dsb->inodes_count += 1; //increasing inode count in superblock

	if (count >= MAX_INODES) {//the block for metadata structure is filled

		printk(KERN_ERR "Maximum number of objects supported by imfs reached");
		return -ENOSPC;
	}

	inode = new_inode(sb);
	
	inode->i_sb = sb;
	inode->i_op = &imfs_inode_ops;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_ino = count;

	imfs_inode = (struct imfs_disk_inode *)(kmalloc(sizeof(struct imfs_disk_inode),GFP_KERNEL));
	imfs_inode->inode_no = inode->i_ino;
	inode->i_private = imfs_inode;
	imfs_inode->mode = mode;

	if (S_ISDIR(mode)) {
		printk(KERN_INFO "New directory creation request\n");
		imfs_inode->dir_children_count = 0;
		inode->i_fop = &imfs_dir_operations;
	} else if (S_ISREG(mode)) {
		printk(KERN_INFO "New file creation request\n");
		imfs_inode->file_size = 0;
		inode->i_fop = &imfs_file_operations;
	}
	
	//Associate a data block with this inode
	imfs_inode->data_block_number = dsb->next_free_block;
	dsb->next_free_block ++; 
	
	//Add it to list of inodes on disk
	
	//struct buffer_head *bh;
	struct imfs_disk_inode *ptr;

	bh = sb_bread(sb,INODE_STORE);
	
	ptr = (struct imfs_inode *)bh->b_data;
	ptr += (dsb->inodes_count-1);
	
	//copy inode and mark dirty
	memcpy(ptr, imfs_inode, sizeof(struct imfs_disk_inode));
	
	mark_buffer_dirty(bh);
	brelse(bh);
	
	//write the dirtied superblock
	bh = sb_bread(sb,0);
	
	//memcpy(bh->b_data, dsb, sizeof(struct imfs_disk_super_block));
	bh->b_data = dsb;
	mark_buffer_dirty(bh);
	brelse(bh);
		
	//Make entry in parent directory. Skip it for now. !! It has to be done
	
	parent_dir_inode = dir->i_private;
	new_record = (struct imfs_dir_record *)kmalloc(sizeof(struct imfs_dir_record),GFP_KERNEL);
	new_record->inode_no = imfs_inode->inode_no;
	strcpy(new_record->filename, dentry->d_name.name);
	
	bh = sb_bread(sb, parent_dir_inode->data_block_number);
	dir_contents_datablock = (struct imfs_dir_record *)bh->b_data;

	/* Navigate to the last record in the directory contents */
	dir_contents_datablock += parent_dir_inode->dir_children_count;

	memcpy(dir_contents_datablock,new_record,sizeof(struct imfs_dir_record));
	
	mark_buffer_dirty(bh);
	brelse(bh);

	parent_dir_inode->dir_children_count++;
	
	//save parent inode to disk
	ret = imfs_inode_save(sb, parent_dir_inode);

	inode_init_owner(inode, dir, mode);
	d_add(dentry, inode);

	return 0;
}

static void imfs_put_super(struct super_block *sb)
{
	printk(KERN_INFO "imfs super block destroyed\n");
}

static struct super_operations const imfs_super_ops = {
	.put_super  = imfs_put_super,
	.drop_inode	= generic_delete_inode,
};


static int imfs_fill_sb(struct super_block *sb, void *data, int silent)
{
	struct inode *root = NULL;
	printk(KERN_INFO "Inside fill_sb.\n"); 

	struct buffer_head *bh,*bh1;

	bh = sb_bread(sb, 0);

	struct imfs_disk_super_block * dsb = (struct imfs_disk_super_block *)(bh->b_data);
	printk(KERN_INFO "[IMFS] Read from disk: Magic: %lx\n",dsb->magic);
    sb->s_magic = dsb->magic;
    sb->s_op = &imfs_super_ops;
    sb->s_fs_info = dsb;
    
	bh1 = sb_bread(sb, INODE_STORE);
	struct imfs_disk_inode * dinode = (struct imfs_disk_inode *)(bh1->b_data);

	root = new_inode(sb);
    if (!root)
    {
		printk(KERN_INFO "inode allocation failed\n");
        return -ENOMEM;
    }

    root->i_ino = 0; 
    root->i_mode = S_IFDIR | 0777; dinode->mode = S_IFDIR | 0777;
    root->i_sb = sb;
    root->i_atime = root->i_mtime = root->i_ctime = CURRENT_TIME;
    
    root->i_op = &imfs_inode_ops;
	root->i_fop = &imfs_dir_operations;
	root->i_private = dinode;
	printk(KERN_INFO "Root dir is : %lu\n",(unsigned long)((struct imfs_disk_inode *)root->i_private)->data_block_number);
    inode_init_owner(root, NULL, S_IFDIR);

    sb->s_root = d_make_root(root);
    if (!sb->s_root)
    {
		printk(KERN_INFO "root creation failed\n");
        return -ENOMEM;
	}
	return 0;

}

static struct dentry *imfs_mount(struct file_system_type *type, int flags,
			char const *dev, void *data)
{
	struct dentry *entry = mount_bdev(type, flags, dev, data, imfs_fill_sb);

	if (IS_ERR(entry))
		printk(KERN_INFO "imfs mounting failed\n");
	else
		printk(KERN_INFO "imfs mounted\n");
	return entry;
}

static struct file_system_type imfs_type = {
	.owner = THIS_MODULE,
	.name = "imfs",
	.mount = imfs_mount,
	.kill_sb = kill_block_super,
	.fs_flags = FS_REQUIRES_DEV
};

static int __init imfs_init(void)
{

	int ret = register_filesystem(&imfs_type);
	if (ret != 0) {
		printk(KERN_INFO "cannot register filesystem\n");
		return ret;
	}

	printk(KERN_INFO "imfs module loaded\n");

	return 0;
}

static void __exit imfs_fini(void)
{
	int ret = unregister_filesystem(&imfs_type);

	if (ret != 0)
		printk(KERN_INFO "cannot unregister filesystem\n");

	printk(KERN_INFO "imfs module unloaded\n");
}

module_init(imfs_init);
module_exit(imfs_fini);

MODULE_LICENSE("GPL");
