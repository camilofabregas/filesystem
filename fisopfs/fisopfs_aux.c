//-------- File name to save filesystem --------//

char *file_name = "fs.fisopfs";

//-------- Aux. functions --------//

int
save_file()
{
	FILE *file = fopen(file_name, "w");
	if (!file)
		return errno;  // Error
	int written_items;
	for (int i = 0; i < MAX_NUMBER_OF_BLOCKS; i++) {
		written_items =
		        fwrite(&BlocksBitmap[i], sizeof(BlocksBitmap[i]), 1, file);
		if (written_items < 0)
			return errno;
	}
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		written_items =
		        fwrite(&InodesBitmap[i], sizeof(InodesBitmap[i]), 1, file);
		if (written_items < 0)
			return errno;
	}
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		written_items =
		        fwrite(&InodeTable[i], sizeof(InodeTable[i]), 1, file);
		if (written_items < 0)
			return errno;
	}
	for (int i = 0; i < MAX_NUMBER_OF_BLOCKS; i++) {
		written_items =
		        fwrite(&DataRegion[i], sizeof(DataRegion[i]), 1, file);
		if (written_items < 0)
			return errno;
	}
	fclose(file);
	return 0;
}

int
load_file(FILE *file)
{
	int read_items = 0;
	for (int i = 0; i < MAX_NUMBER_OF_BLOCKS; i++) {
		read_items =
		        fread(&BlocksBitmap[i], sizeof(BlocksBitmap[i]), 1, file);
		if (read_items < 0)
			return errno;
	}
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		read_items =
		        fread(&InodesBitmap[i], sizeof(InodesBitmap[i]), 1, file);
		if (read_items < 0)
			return errno;
	}
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		read_items =
		        fread(&InodeTable[i], sizeof(InodeTable[i]), 1, file);
		if (read_items < 0)
			return errno;
	}
	for (int i = 0; i < MAX_NUMBER_OF_BLOCKS; i++) {
		read_items =
		        fread(&DataRegion[i], sizeof(DataRegion[i]), 1, file);
		if (read_items < 0)
			return errno;
	}
	return 0;
}

void
initialize_bitmap(bool *bitmap)
{
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		bitmap[i] = false;
	}
}

void
initialize_inode(unsigned int index,
                 enum FileSystemType type,
                 unsigned int block_index)
{
	InodesBitmap[index] = true;

	Inode *inode = &InodeTable[index];
	inode->uid = getuid();
	inode->gid = getuid();
	inode->type = type;
	inode->size = 0;
	inode->creation_time = time(NULL);
	inode->last_access_time = time(NULL);
	inode->last_modification_time = time(NULL);
	inode->assigned_block = block_index;
}

void
initialize_file_block(unsigned int index)
{
	BlocksBitmap[index] = true;

	Block *file_block = &DataRegion[index];
	file_block->type = FS_FILE;
	memset(file_block->file.content, 0, sizeof(file_block->file.content));
	file_block->file.content[0] = '\0';
}

void
initialize_dir_block(unsigned int index)
{
	BlocksBitmap[index] = true;
	DataRegion[index].type = FS_DIRECTORY;
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		DataRegion[index].dir.childs[i].assigned_inode = MOUNT_INODE_INDEX;
	}
}

void
init_file()
{
	initialize_bitmap(InodesBitmap);
	initialize_bitmap(BlocksBitmap);
	// Assign mount / directory
	initialize_inode(MOUNT_INODE_INDEX, FS_DIRECTORY, MOUNT_BLOCK_INDEX);
	initialize_dir_block(MOUNT_BLOCK_INDEX);
}

unsigned int
find_free_index(const bool *bitmap)
{
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		if (bitmap[i] == false) {
			return i;
		}
	}

	// TODO handle better error value
	return UINT_MAX;
}

DirectoryEntry *
find_directory_entry(unsigned int dir_inode, const char *path)
{
	Block dir_block = DataRegion[InodeTable[dir_inode].assigned_block];
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		DirectoryEntry actual_entry = dir_block.dir.childs[i];
		if (strcmp(path, actual_entry.filename) == 0) {
			return &DataRegion[InodeTable[dir_inode].assigned_block]
			                .dir.childs[i];
		}
	}

	return NULL;
}

void
debug_structs()
{
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		printf("%i: Inode used: %i\n", i, InodesBitmap[i]);
		printf("%i: Block Used: %i\n", i, BlocksBitmap[i]);
	}
}

void
debug_dir_entries(unsigned int block_index)
{
	Block block = DataRegion[block_index];
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		DirectoryEntry actual_child = block.dir.childs[i];

		printf("Filename: %s\n", actual_child.filename);
		printf("Inode: %i\n", actual_child.assigned_inode);
	}
}

int
separate_path(const char *path,
              char dirs[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE])
{
	const char *delimiter = "/";
	char *temp = malloc(strlen(path) + 1);
	char *token;
	int count = 0;

	if (temp == NULL) {
		return -ENOMEM;  // Memory allocation error
	}

	strcpy(temp, path);
	token = strtok(temp, delimiter);

	while (token != NULL) {
		if (*token != '\0') {
			if (count >= MAX_NESTED_DIRECTORIES) {
				free(temp);
				return -1;  // Maximum nested directories reached
			}

			strcpy(dirs[count], token);
			count++;
		}
		token = strtok(NULL, delimiter);
	}

	free(temp);
	return count;
}

int
get_nested_dir_inode(const char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE],
                     int count)
{
	unsigned int initial_dir_inode = MOUNT_INODE_INDEX;
	for (int i = 0; i < count; i++) {
		if (InodeTable[initial_dir_inode].type == FS_FILE) {
			return -ENOENT;
		}

		const Block *actual_block =
		        &DataRegion[InodeTable[initial_dir_inode].assigned_block];
		for (int j = 0; j < MAX_NUMBERS_OF_INODES; j++) {
			if (strcmp(actual_block->dir.childs[j].filename,
			           paths[i]) == 0) {
				initial_dir_inode =
				        actual_block->dir.childs[j].assigned_inode;
			}
		}
	}

	return initial_dir_inode;
}

void
delete_directory_entries(unsigned int inode_index)
{
	Block *dir_block = &DataRegion[InodeTable[inode_index].assigned_block];
	for (int i = 0; i < MAX_NUMBERS_OF_INODES; i++) {
		DirectoryEntry *actual_child = &dir_block->dir.childs[i];
		if (actual_child->assigned_inode != MOUNT_INODE_INDEX) {
			InodesBitmap[actual_child->assigned_inode] = false;
			BlocksBitmap[InodeTable[actual_child->assigned_inode].assigned_block] =
			        false;

			if (InodeTable[actual_child->assigned_inode].type ==
			    FS_DIRECTORY) {
				delete_directory_entries(
				        actual_child->assigned_inode);
			}

			actual_child->assigned_inode = MOUNT_INODE_INDEX;
			memset(actual_child->filename,
			       0,
			       sizeof(actual_child->filename));
			actual_child->filename[0] = '\0';
		}
	}
}
