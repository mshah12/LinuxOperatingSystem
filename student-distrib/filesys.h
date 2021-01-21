#ifndef FILESYS_H
#define FILESYS_H

#include "lib.h"
#include "types.h"
#include "syscall.h"

#define FILENAME_LEN 32
#define blockSize 4096
#define fileArraySize 8

typedef struct dentry{
    uint8_t filename[FILENAME_LEN];
    uint32_t filetype;
    uint32_t inode_num;
    uint8_t reserved[24];
}__attribute__ ((packed)) dentry;

typedef struct boot_block{
    uint32_t dir_count;
    uint32_t inode_count;
    uint32_t data_count;
    uint8_t reserved[52];
    dentry direntries[63];
} __attribute__ ((packed)) boot_block;

typedef struct inode{
    uint32_t length;
    uint32_t data_block_num[1023];
}__attribute__ ((packed)) inode;

typedef struct dataBlock{
    uint8_t data_byte[4096];
}__attribute__ ((packed)) dataBlock;

void init_filesys(uint32_t fileSysPtrLocal);

int32_t read_dentry_by_name(const uint8_t* fname, dentry* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, int8_t* buf, uint32_t length);

//reads bytes of data from file into buffer
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
//does nothing, returns -1
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
//initalize any structures, return 0
int32_t file_open(const uint8_t* filename);
//undoes the doings of file_open(), returns 0
int32_t file_close(int32_t fd);
//reads filename, including "."
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
//does nothing, returns -1
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);
//opens a directory file, returns 0
int32_t dir_open(const uint8_t* filename);
//does nothing, returns 0
int32_t dir_close(int32_t fd);

extern inode* inodePtr;
#endif
