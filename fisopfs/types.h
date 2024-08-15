#ifndef _TYPES_H_
#define _TYPES_H_

//-------- Constants --------//

// Max size of file content
#define MAX_FILE_CONTENT_SIZE 1000

// Max size of file/directory name
#define MAX_FILE_NAME_SIZE 100

// Max number of blocks in the Data Region
#define MAX_NUMBER_OF_BLOCKS 20

// Max number of Inodes in the Inode Table
#define MAX_NUMBERS_OF_INODES 20

// Inode index for the File System mount
#define MOUNT_INODE_INDEX 0

// Block index for the File System mount
#define MOUNT_BLOCK_INDEX 0

// Max number of nested directories allowed
#define MAX_NESTED_DIRECTORIES 4


//-------- Types --------//

// Represent the type of Inode/Block
enum FileSystemType { FS_FILE, FS_DIRECTORY };

// Struct that represents a file memory block in the file system
typedef struct FileBlock {
	char content[MAX_FILE_CONTENT_SIZE];
} FileBlock;

// Struct that represents an entry (file or directory) inside a Directory
typedef struct DirectoryEntry {
	char filename[MAX_FILE_NAME_SIZE];
	unsigned int assigned_inode;
} DirectoryEntry;

// Struct that represents a directory memory block in the file system
typedef struct DirBlock {
	DirectoryEntry childs[MAX_NUMBERS_OF_INODES];
} DirBlock;

// Struct that represents a block of memory
typedef struct Block {
	enum FileSystemType type;
	union {
		FileBlock file;
		DirBlock dir;
	};
} Block;

// The data region with all the blocks of the file system
Block DataRegion[MAX_NUMBER_OF_BLOCKS];

// Bitmap for the Blocks
bool BlocksBitmap[MAX_NUMBER_OF_BLOCKS];

// Struct that represents the Inode of the file system
// Used for saving metadata
typedef struct Inode {
	enum FileSystemType type;
	uid_t uid;
	gid_t gid;
	unsigned int size;
	time_t creation_time;
	time_t last_access_time;
	time_t last_modification_time;
	unsigned int assigned_block;
} Inode;

// Array containing all the Inodes of the file system
Inode InodeTable[MAX_NUMBERS_OF_INODES];

// Bitmap for the Inodes
bool InodesBitmap[MAX_NUMBERS_OF_INODES];

#endif  // _TYPES_H_