#include "filesys.h"
#include "syscall.h"

//pointer to boot block
boot_block* bb;
//inode start
inode* inodePtr;
//Data block start
dataBlock* dataPtr;
//Global Process control block array, we will probably delete this after checkpoint 2? 
//FD_Member fileArray[fileArraySize];
//fileSysPtr
uint32_t fileSysPtr;
//file operations table pointer
//int32_t (*file_func[4]) ()
//directory operations table pointer

// void init_filesys(uint32_t fileSysPtrLocal):
// DESCRIPTION: Initializes the file system
// INPUTS: fileSysPtrLocal - starting address of the file system
// OUTPUTS: none
// RETURN VALUE: none
// SIDE EFFECTS: all global variables in filesys.c are updated
void init_filesys(uint32_t fileSysPtrLocal)
{
    uint32_t dataPtrOffset;
	fileSysPtr = fileSysPtrLocal;
    bb = (boot_block*) fileSysPtr;
    inodePtr = (inode*) (fileSysPtr + blockSize);
    //Find starting address of data block
    dataPtrOffset = bb->inode_count;
	dataPtrOffset = (blockSize * dataPtrOffset);
    dataPtr = (dataBlock*)((uint32_t)inodePtr + dataPtrOffset);
}
// int32_t read_dentry_by_name(const uint8_t* fname, dentry* dentry):
// DESCRIPTION: Checks if there is a directory with the name "fname"
// INPUTS: fname - The name of the file
//         dentry - Directory entry to be updated
// OUTPUTS: none
// RETURN VALUE: 0 - Returned if directory entry with "fname" is found
//               1 - Returned if directory entry with "fname" is not found
// SIDE EFFECTS: none
int32_t read_dentry_by_name(const uint8_t* fname, dentry* dentry)
{
    int index;
    if(fname[0] == '\0') 
    {
        curDirIndex = -1;
        return -1;
    }
	//63: There are max 64 possible directory entries
    for(index = 0; index < 63; index++)
    {
        char tempArray[FILENAME_LEN + 1];
        memcpy((int8_t *)tempArray, (int8_t *)&bb->direntries[index].filename[0], sizeof(char) * 32);
        tempArray[32] = '\0';
        if(strlen((int8_t*)tempArray) != strlen((int8_t*) fname)) continue;
        if(strncmp((int8_t*)tempArray, (int8_t*)fname, (uint32_t)strlen((int8_t*)&bb->direntries[index].filename[0])) == 0)
        {
            dentry->filetype = bb->direntries[index].filetype;
            dentry->inode_num = bb->direntries[index].inode_num;
            strcpy((int8_t*)&dentry->filename[0], (int8_t*)&bb->direntries[index].filename[0]);
            curDirIndex = index;
            return 0;
        }
    }
    curDirIndex = -1;
    return -1;
}
// int32_t read_dentry_by_index(uint32_t index, dentry* dentry):
// DESCRIPTION: Updates dentry with directory entry at index "index"
// INPUTS: index - The index number for the direntries array
//         dentry - Directory entry to be updated
// OUTPUTS: none
// RETURN VALUE: 0 - Returned if directory entry with "fname" is found
//               1 - Returned if index is out of bounds
// SIDE EFFECTS: none
int32_t read_dentry_by_index(uint32_t index, dentry* dentry)
{
    if(0 > index || 63 <= index)
    {
        return -1;
    }
    else
    {
		//Index into direntries structure
        dentry->filetype = bb->direntries[index].filetype;
        dentry->inode_num = bb->direntries[index].inode_num;
        strcpy((int8_t*)dentry->filename, (int8_t*)bb->direntries[index].filename);
        return 0;
    }
}
// int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length):
// DESCRIPTION: reads "length" number of bytes from a file into the buffer
// INPUTS: inode - The index number for inode
//         offset - The start address of the file
//         buf - buffer to store the data into
//         length - number of bytes to read
// OUTPUTS: none
// RETURN VALUE: -1 - Returned if pointers are NULL
//               length - number of bytes read are returned
// SIDE EFFECTS: none
int32_t read_data(uint32_t inode, uint32_t offset, int8_t* buf, uint32_t length)
{
	int originalLength = length;
/*	int i = 0;
	//Find block index
    int dataBlockIndex = offset / blockSize;
    while(length > 0)
	{
		//If length greater than block size
    if(length > blockSize){
    memcpy((int8_t*)&buf[i],(int8_t*)&dataPtr[inodePtr[inode].data_block_num[dataBlockIndex]].data_byte[0],blockSize);
    length -= blockSize;
    i+=blockSize;
    dataBlockIndex +=1;
    }else{
		//If within block size length
	uint32_t dataBlockIndexVal = inodePtr[inode].data_block_num[dataBlockIndex];
    memcpy((int8_t*)&buf[i],(int8_t*)&dataPtr[dataBlockIndexVal].data_byte[offset % blockSize],length);  
	break;
    }
    }*/
	
	int tempPos = offset%4096;
	int dataBlockIndex = offset/4096;
	int dataByteIndex = tempPos;
	int i = 0;
	int remainder = 4096 - dataByteIndex;
	if(tempPos + length > 4096)
	{
		length = length - remainder;
		while(1){
			memcpy((int8_t*)&buf[i],(int8_t*)&dataPtr[inodePtr[inode].data_block_num[dataBlockIndex]].data_byte[dataByteIndex],remainder);
			i+= remainder;
			if(length <= 0) break;
			dataByteIndex = 0;
			dataBlockIndex++;
			if(length <= 4096)
			{
				remainder = length;
				length = 0;
			}else{
				remainder = 4096;
				length = length -4096;
			}
		}
	}else{
		    memcpy((int8_t*)&buf[0],(int8_t*)&dataPtr[inodePtr[inode].data_block_num[dataBlockIndex]].data_byte[tempPos],length);
	}
	/*
	//Which 4kb data block we need to go into
	uint32_t dataBlockIndex = offset/4096;
	//which byte inside said data block 
	uint32_t dataByteIndex = offset%4096;
	int i;
	//if overflows into next data block
	if(length + dataByteIndex > 4096)
	{
		int bufTrack = 0;
		int indexOff = 4096-dataByteIndex;
		for(i = 0; i < (length+dataByteIndex)/4096; i++)
		{
			uint32_t dataBlockIndexVal = inodePtr[inode].data_block_num[dataBlockIndex + i];
			memcpy((int8_t*)&buf[bufTrack],(int8_t*)&dataPtr[dataBlockIndexVal].data_byte[dataByteIndex],indexOff); 
			if()
		}
	}
	*/
	return originalLength; 
    
}
// int32_t file_read(int32_t fd, void* buf, int32_t nbytes):
// DESCRIPTION: reads file indexed at fd into the buffer
// INPUTS: fd - The index number for the file array
//         buf - buffer to store the data into
//         length - number of bytes to read
// OUTPUTS: none
// RETURN VALUE: -1 - Returned if pointers are NULL, fd is out of bounds, or inode length is greater
//                0 - Returned if file is successfully read
// SIDE EFFECTS: none
int32_t file_read(int32_t fd, void* buf, int32_t nbytes)
{
    uint32_t new_pos;
	if(buf == NULL || nbytes <= 0) return -1;
    //NOTE: Every read system call should update "file position" member
    if(fd <= 1 || fd >= 8)
    {
        return -1;
    }
    else
    {
		//Grab the inodeblock
        inode inodeBlock = inodePtr[fileArray[fd].inode];
        if(inodeBlock.length <= fileArray[fd].filePosition)
        {
            return 0;
        }
		if(fileArray[fd].filePosition + nbytes >= inodeBlock.length)
		{
			int originalNbytes = nbytes;
			int8_t* tempBuf = (int8_t*) buf;
			nbytes = inodeBlock.length - fileArray[fd].filePosition;
			memset((int8_t * )&tempBuf[nbytes],'\0', originalNbytes - nbytes);
		}
		//Call read data and read current block
        new_pos = read_data(fileArray[fd].inode, fileArray[fd].filePosition, (int8_t *)buf, nbytes);
        fileArray[fd].filePosition += new_pos;
        return new_pos;
    }
	return 0;
}
// int32_t file_write(int32_t fd, const void* buf, int32_t nbytes):
// DESCRIPTION: Does nothing
// INPUTS: fd - The index number for the file array
//         buf - buffer to store the data into
//         length - number of bytes to write
// OUTPUTS: none
// RETURN VALUE: -1 - Returned as function does nothing
// SIDE EFFECTS: none
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
{
	if(buf == NULL || nbytes <= 0) return -1;
    return -1;
}
// int32_t file_open(const uint8_t* filename):
// DESCRIPTION: Opens file by updating file array
// INPUTS: filename - name of the file to be opened
// OUTPUTS: none
// RETURN VALUE: -1 - Returned if file cannot be opened
//               fd - Returns index of the file in the file array
// SIDE EFFECTS: file array is updated
int32_t file_open(const uint8_t* filename)
{
    return 0;
}
// int32_t file_close(int32_t fd):
// DESCRIPTION: Closes file by updating file array
// INPUTS: fd - index of file in file array
// OUTPUTS: none
// RETURN VALUE: 0 - Returned to indicate success
// SIDE EFFECTS: file array is updated
int32_t file_close(int32_t fd)
{
    if(fd == 1 || fd == 0 || fd > fileArraySize - 1 || fd < 0)
    {
        return -1;
    }
    return 0;
}
// int32_t dir_read(int32_t fd, void* buf, int32_t nbytes):
// DESCRIPTION: Reads the file name of the directory
// INPUTS: fd - The index number for the file array
//         buf - buffer to store the data into
//         length - number of bytes to write
// OUTPUTS: none
// RETURN VALUE: filePosition - Returns file position if directory read is executed successfully
//               0 i Returned if direactory is cannot be read
// SIDE EFFECTS: file array is updated
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes)
{
    dentry tempDentry;

    if(fd == 1 || fd == 0 || fd > fileArraySize - 1 || fd < 0)
    {
        return -1;
    }
    //filePosition is used as directoryIndex
    if(fileArray[fd].filePosition >= bb->dir_count)
    {
        return 0;
    }
    read_dentry_by_index(fileArray[fd].filePosition, &tempDentry);
    strncpy((int8_t*) buf, (int8_t*)&tempDentry.filename[0] , nbytes);
    fileArray[fd].filePosition +=1;
    return nbytes;
}
// int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes):
// DESCRIPTION: Does nothing
// INPUTS: fd - The index number for the file array
//         buf - buffer to store the data into
//         length - number of bytes to write
// OUTPUTS: none
// RETURN VALUE: -1 - Returned as function does nothing
// SIDE EFFECTS: None
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes)
{
    if(fd == 1 || fd == 0 || fd > fileArraySize - 1 || fd < 0)
    {
        return -1;
    }
    return -1;
}
// int32_t dir_open(const uint8_t* filename):
// DESCRIPTION: Opens the directory
// INPUTS: filename - the name of the file to be opened
// OUTPUTS: none
// RETURN VALUE: -1 - Returned if directory is not opened succesfully
//               0 - Returned if function executes sucessfully
// SIDE EFFECTS: file array is updated
int32_t dir_open(const uint8_t* filename)
{
    return 0;
}
// int32_t dir_close(int32_t fd):
// DESCRIPTION: Closes the directory
// INPUTS: fd - index of the directory to be closed
// OUTPUTS: none
// RETURN VALUE: - 0 - Returned to indicate success
// SIDE EFFECTS: file array is updated
int32_t dir_close(int32_t fd)
{
    if(fd == 1 || fd == 0 || fd > fileArraySize - 1 || fd < 0)
    {
        return -1;
    }
    return 0;
}
