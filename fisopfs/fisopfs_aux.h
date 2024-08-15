#ifndef _FISOPFS_AUX_H_
#define _FISOPFS_AUX_H_

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include "types.h"
#include "fisopfs_aux.c"

// Save filesystem to a file before exiting/flushing.
int save_file();

// Load filesystem from a file (if it exists) afte init.
int load_file(FILE *file);

// Initialize all structs and create filesystem file.
void init_file();

// Find free index in a bitmap
unsigned int find_free_index(const bool *bitmap);

// Initialize bitmap with all false values
void initialize_bitmap(bool *bitmap);

// Initialize Inode with all default values
void initialize_inode(unsigned int index,
                      enum FileSystemType type,
                      unsigned int block_index);

// Initialize a file block with the default values
void initialize_file_block(unsigned int index);

// Initialize a directory block with the default values
void initialize_dir_block(unsigned int index);

// TODO Only for debugging
//  This functions are only used for debugging
void debug_structs();
void debug_dir_entries(unsigned int block_index);

// Find a directory entry in the directory that matches the path
// If no directory is found, return NULL
DirectoryEntry *find_directory_entry(unsigned int dir_inode, const char *path);

// Separate the path by the '/' char and return the number of elements in the
// dirs array
int separate_path(const char *path,
                  char dirs[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE]);

// Based on a list of filename, finds the nested directory inode index
int get_nested_dir_inode(const char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE],
                         int count);

// Deletes all the directory entries inside a Directory, using the Inode index
void delete_directory_entries(unsigned int inode_index);

#endif  // _FISOPFS_AUX_H_