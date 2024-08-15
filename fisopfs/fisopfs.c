#define FUSE_USE_VERSION 30

#include "fisopfs_aux.h"


//-------- FUSE functions --------//

static void
fisopfs_destroy(void *data)
{
	printf("[debug] fisopfs_destroy\n");
	if (save_file() < 0)
		printf("[debug] fisopfs_destroy - error saving filesystem "
		       "file\n");
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_flush\n");
	int save_result = save_file();
	if (save_result < 0)
		printf("[debug] fisopfs_flush - error saving filesystem "
		       "file\n");
	return save_result;
}

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);

	const Inode *file_inode = NULL;
	if (strcmp(path, "/") == 0) {
		file_inode = &InodeTable[MOUNT_INODE_INDEX];
	} else {
		char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
		int count = separate_path(path, paths);
		if (count < 0)
			return count;
		int parent_inode_index = get_nested_dir_inode(paths, count - 1);
		DirectoryEntry *dir_entry =
		        find_directory_entry(parent_inode_index, paths[count - 1]);

		if (dir_entry == NULL) {
			return -ENOENT;
		}

		file_inode = &InodeTable[dir_entry->assigned_inode];
	}

	st->st_uid = file_inode->uid;
	st->st_gid = file_inode->gid;
	if (file_inode->type == FS_FILE) {
		st->st_mode = __S_IFREG | 0644;
		st->st_nlink = 1;  // TODO Not saved in Inode
		st->st_size = file_inode->size;
	} else {
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;  // TODO Not saved in Inode
	}
	st->st_atime = file_inode->last_access_time;
	st->st_mtime = file_inode->last_modification_time;
	st->st_ctime = file_inode->creation_time;

	return 0;
}

static void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] fisop_init\n");
	FILE *file = fopen(file_name, "r");
	if (file) {
		if (load_file(file) < 0) {
			printf("[debug] fisop_init - error loading filesystem "
			       "file\n");
			return NULL;
		}
		fclose(file);
	} else {
		init_file();
	}
	return 0;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s\n", path);

	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	int count = separate_path(path, paths);
	if (count < 0)
		return count;
	int parent_inode_index = get_nested_dir_inode(paths, count - 1);
	unsigned int parent_block_index =
	        InodeTable[parent_inode_index].assigned_block;

	DirectoryEntry *dir_entry =
	        find_directory_entry(parent_inode_index, paths[count - 1]);
	if (dir_entry != NULL) {
		return -EEXIST;  // Directory already exists
	}

	// Find an available inode and block
	unsigned int inode_index = find_free_index(InodesBitmap);
	unsigned int block_index = find_free_index(BlocksBitmap);
	if (inode_index == UINT_MAX || block_index == UINT_MAX) {
		return -ENOSPC;
	}

	// Initialize the Inode and Data Block
	initialize_inode(inode_index, FS_DIRECTORY, block_index);
	initialize_dir_block(block_index);

	// Searches a free space in parent directory entries
	// TODO Handle no space
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		if (DataRegion[parent_block_index].dir.childs[i].assigned_inode ==
		    MOUNT_INODE_INDEX) {
			DataRegion[parent_block_index].dir.childs[i].assigned_inode =
			        inode_index;
			// +1 used to not copy the /
			strcpy(DataRegion[parent_block_index].dir.childs[i].filename,
			       paths[count - 1]);
			break;
		}
	}

	return 0;
}

int
fisopfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	printf("[debug] fisopfs_mknod - path: %s\n", path);

	unsigned int inode_index = find_free_index(InodesBitmap);
	unsigned int block_index = find_free_index(BlocksBitmap);
	if (inode_index == UINT_MAX || block_index == UINT_MAX) {
		return -ENOSPC;
	}


	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	int count = separate_path(path, paths);
	if (count < 0)
		return count;
	int parent_inode_index = get_nested_dir_inode(paths, count - 1);
	unsigned int parent_block_index =
	        InodeTable[parent_inode_index].assigned_block;

	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		if (DataRegion[parent_block_index].dir.childs[i].assigned_inode ==
		    MOUNT_INODE_INDEX) {
			DataRegion[parent_block_index].dir.childs[i].assigned_inode =
			        inode_index;
			// +1 used to not copy the /
			strcpy(DataRegion[parent_block_index].dir.childs[i].filename,
			       paths[count - 1]);
			break;
		}
	}

	// Initialize Inode
	initialize_inode(inode_index, FS_FILE, block_index);

	// Initialize Block
	initialize_file_block(block_index);

	return 0;
}

static int
fisopfs_open(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_open - path: %s\n", path);

	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	int count = separate_path(path, paths);
	if (count < 0)
		return count;
	int parent_inode_index = get_nested_dir_inode(paths, count - 1);
	DirectoryEntry *dir_entry =
	        find_directory_entry(parent_inode_index, paths[count - 1]);

	if (dir_entry == NULL) {
		return -ENOENT;  // File not found
	}

	// Get the assigned inode of the file
	unsigned int inode_index = dir_entry->assigned_inode;

	// Store the file inode index in the fuse_file_info structure
	fi->fh = inode_index;

	return 0;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	int count = separate_path(path, paths);
	if (count < 0)
		return count;
	int parent_inode_index = get_nested_dir_inode(paths, count - 1);
	DirectoryEntry *dir_entry =
	        find_directory_entry(parent_inode_index, paths[count - 1]);

	if (dir_entry == NULL) {
		return -ENOENT;
	}

	Inode *file_inode = &InodeTable[dir_entry->assigned_inode];
	const Block *file_block = &DataRegion[file_inode->assigned_block];

	if (offset + size > file_inode->size)
		size = file_inode->size - offset;

	size = size > 0 ? size : 0;

	strncpy(buffer, file_block->file.content + offset, size);

	file_inode->last_access_time = time(NULL);
	return size;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);

	// Directories '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	int count = separate_path(path, paths);
	if (count < 0)
		return count;
	int parent_inode_index = get_nested_dir_inode(paths, count);
	unsigned int parent_block_index =
	        InodeTable[parent_inode_index].assigned_block;

	if (parent_inode_index < 0) {
		return -parent_inode_index;
	}

	const Block *dir_block = &DataRegion[parent_block_index];
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		const DirectoryEntry *actual_entry = &dir_block->dir.childs[i];
		if (actual_entry->assigned_inode != MOUNT_INODE_INDEX) {
			filler(buffer, actual_entry->filename, NULL, 0);
		}
	}
	return 0;
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir - path: %s\n", path);

	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	int count = separate_path(path, paths);
	int parent_inode_index = get_nested_dir_inode(paths, count - 1);
	DirectoryEntry *dir_entry =
	        find_directory_entry(parent_inode_index, paths[count - 1]);

	if (dir_entry == NULL) {
		return -ENOENT;  // Directory not found
	}

	// Get the assigned inode of the directory
	unsigned int inode_index = dir_entry->assigned_inode;


	// Free all bitmap references
	InodesBitmap[inode_index] = false;
	BlocksBitmap[InodeTable[inode_index].assigned_block] = false;

	// Free all childs
	delete_directory_entries(inode_index);

	// Free DirectoryEntry in parent folder
	dir_entry->assigned_inode = MOUNT_INODE_INDEX;
	memset(dir_entry->filename, 0, sizeof(dir_entry->filename));
	dir_entry->filename[0] = '\0';

	return 0;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink - path: %s\n", path);


	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	int count = separate_path(path, paths);
	int parent_inode_index = get_nested_dir_inode(paths, count - 1);
	DirectoryEntry *dir_entry =
	        find_directory_entry(parent_inode_index, paths[count - 1]);


	if (dir_entry == NULL) {
		return -ENOENT;
	}

	// Get the inode index of the file
	unsigned int inode_index = dir_entry->assigned_inode;

	// Free all bitmap references
	InodesBitmap[inode_index] = false;
	BlocksBitmap[InodeTable[inode_index].assigned_block] = false;

	// Free DirectoryEntry in parent folder
	dir_entry->assigned_inode = MOUNT_INODE_INDEX;
	memset(dir_entry->filename, 0, sizeof(dir_entry->filename));
	dir_entry->filename[0] = '\0';

	return 0;
}

static int
fisopfs_utime(const char *path, struct utimbuf *buf)
{
	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	int count = separate_path(path, paths);

	int parent_inode_index = get_nested_dir_inode(paths, count - 1);


	DirectoryEntry *dir_entry =
	        find_directory_entry(parent_inode_index, paths[count - 1]);

	if (dir_entry == NULL) {
		return -ENOENT;
	} else {
		const Inode *inode = &InodeTable[dir_entry->assigned_inode];
		buf->actime = inode->last_access_time;
		buf->modtime = inode->last_modification_time;

		return 0;
	}
}

static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	int count = separate_path(path, paths);
	int parent_inode_index = get_nested_dir_inode(paths, count - 1);
	DirectoryEntry *dir_entry =
	        find_directory_entry(parent_inode_index, paths[count - 1]);


	if (dir_entry == NULL) {
		return -ENOENT;
	}

	Inode *file_inode = &InodeTable[dir_entry->assigned_inode];
	Block *file_block = &DataRegion[file_inode->assigned_block];

	if (offset + size > file_inode->size) {
		// Increase the file size if necessary
		file_inode->size = offset + size;
	}


	// Copy the content from the buffer to the file block
	strncpy(file_block->file.content + offset, buf, size);

	// Remove the newline character from the buffer, if present
	if (size > 0 && buf[size - 1] == '\n') {
		file_block->file.content[offset + size - 1] = '\0';
	}

	// Update the timestamps
	file_inode->last_access_time = time(NULL);
	file_inode->last_modification_time = time(NULL);

	return size;
}


//-------- Not implemented FUSE functions --------//

static int
fisopfs_readlink(const char *, char *, size_t)
{
	printf("[debug] fisopfs_readlink - not implemented\n");
	return 0;
}

static int
fisopfs_symlink(const char *, const char *)
{
	printf("[debug] fisopfs_symlink - not implemented\n");
	return 0;
}

static int
fisopfs_link(const char *, const char *)
{
	printf("[debug] fisopfs_link - not implemented\n");
	return 0;
}

static int
fisopfs_chmod(const char *, mode_t)
{
	printf("[debug] fisopfs_chmod - not implemented\n");
	return 0;
}

static int
fisopfs_chown(const char *, uid_t, gid_t)
{
	printf("[debug] fisopfs_chown - not implemented\n");
	return 0;
}

static int
fisopfs_truncate(const char *, off_t)
{
	printf("[debug] fisopfs_truncate - not implemented\n");
	return 0;
}

static int
fisopfs_statfs(const char *, struct statvfs *)
{
	printf("[debug] fisopfs_statfs - not implemented\n");
	return 0;
}

static int
fisopfs_release(const char *, struct fuse_file_info *)
{
	printf("[debug] fisopfs_release - not implemented\n");
	return 0;
}

static int
fisopfs_fsync(const char *, int, struct fuse_file_info *)
{
	printf("[debug] fisopfs_fsync - not implemented\n");
	return 0;
}

static int
fisopfs_setxattr(const char *, const char *, const char *, size_t, int)
{
	printf("[debug] fisopfs_setxattr - not implemented\n");
	return 0;
}

static int
fisopfs_getxattr(const char *, const char *, char *, size_t)
{
	printf("[debug] fisopfs_getxattr - not implemented\n");
	return 0;
}

static int
fisopfs_listxattr(const char *, char *, size_t)
{
	printf("[debug] fisopfs_listxattr - not implemented\n");
	return 0;
}

static int
fisopfs_removexattr(const char *, const char *)
{
	printf("[debug] fisopfs_removexattr - not implemented\n");
	return 0;
}

static int
fisopfs_releasedir(const char *, struct fuse_file_info *)
{
	printf("[debug] fisopfs_releasedir - not implemented\n");
	return 0;
}

static int
fisopfs_fsyncdir(const char *, int, struct fuse_file_info *)
{
	printf("[debug] fisopfs_fsyncdir - not implemented\n");
	return 0;
}

static int
fisopfs_access(const char *, int)
{
	printf("[debug] fisopfs_access - not implemented\n");
	return 0;
}

static int
fisopfs_ftruncate(const char *, off_t, struct fuse_file_info *)
{
	printf("[debug] fisopfs_ftruncate - not implemented\n");
	return 0;
}

static int
fisopfs_lock(const char *, struct fuse_file_info *, int cmd, struct flock *)
{
	printf("[debug] fisopfs_lock - not implemented\n");
	return 0;
}

static int
fisopfs_bmap(const char *, size_t blocksize, uint64_t *idx)
{
	printf("[debug] fisopfs_bmap - not implemented\n");
	return 0;
}

static int
fisopfs_ioctl(const char *,
              int cmd,
              void *arg,
              struct fuse_file_info *,
              unsigned int flags,
              void *data)
{
	printf("[debug] fisopfs_ioctl - not implemented\n");
	return 0;
}

static int
fisopfs_poll(const char *,
             struct fuse_file_info *,
             struct fuse_pollhandle *ph,
             unsigned *reventsp)
{
	printf("[debug] fisopfs_poll - not implemented\n");
	return 0;
}

static int
fisopfs_flock(const char *, struct fuse_file_info *, int op)
{
	printf("[debug] fisopfs_flock - not implemented\n");
	return 0;
}

static int
fisopfs_fallocate(const char *, int, off_t, off_t, struct fuse_file_info *)
{
	printf("[debug] fisopfs_fallocate - not implemented\n");
	return 0;
}


//-------- FUSE operations --------//

static struct fuse_operations operations = { .destroy = fisopfs_destroy,
	                                     .flush = fisopfs_flush,
	                                     .getattr = fisopfs_getattr,
	                                     .init = fisopfs_init,
	                                     .mkdir = fisopfs_mkdir,
	                                     .mknod = fisopfs_mknod,
	                                     .open = fisopfs_open,
	                                     .read = fisopfs_read,
	                                     .readdir = fisopfs_readdir,
	                                     .rmdir = fisopfs_rmdir,
	                                     .unlink = fisopfs_unlink,
	                                     .utime = fisopfs_utime,
	                                     .write = fisopfs_write,

	                                     // Not implemented
	                                     .readlink = fisopfs_readlink,
	                                     .symlink = fisopfs_symlink,
	                                     .link = fisopfs_link,
	                                     .chmod = fisopfs_chmod,
	                                     .chown = fisopfs_chown,
	                                     .truncate = fisopfs_truncate,
	                                     .statfs = fisopfs_statfs,
	                                     .release = fisopfs_release,
	                                     .fsync = fisopfs_fsync,
	                                     .setxattr = fisopfs_setxattr,
	                                     .getxattr = fisopfs_getxattr,
	                                     .listxattr = fisopfs_listxattr,
	                                     .removexattr = fisopfs_removexattr,
	                                     .releasedir = fisopfs_releasedir,
	                                     .fsyncdir = fisopfs_fsyncdir,
	                                     .access = fisopfs_access,
	                                     .ftruncate = fisopfs_ftruncate,
	                                     .lock = fisopfs_lock,
	                                     .bmap = fisopfs_bmap,
	                                     .ioctl = fisopfs_ioctl,
	                                     .poll = fisopfs_poll,
	                                     .flock = fisopfs_flock,
	                                     .fallocate = fisopfs_fallocate };


//-------- Main --------//

int
main(int argc, char *argv[])
{
	if (argc == 4) {  // Custom FS data file name
		strcat(argv[3], ".fisopfs");
		file_name = argv[3];
		argc--;
		char *new_argv[4] = { argv[0], argv[1], argv[2], argv[4] };
		*argv = *new_argv;
	}
	return fuse_main(argc, argv, &operations, NULL);
}
