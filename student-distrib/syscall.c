#include "syscall.h"
#include "filesys.h"
#include "rtc.h"
#include "interrupt.h"
#include "paging.h"
#include "terminal_driver.h"
#include "terminals.h"

#define VIDEO       0x0B8000

//Current PCB structure that we are pointing to
PCB* currPCBPtr = (PCB*) startPCBAddr;
PCB* t1TopProgram = (PCB*) startPCBAddr;
PCB* t2TopProgram = NULL;
PCB* t3TopProgram = NULL;
PCB* t1BottomProgram = (PCB*) startPCBAddr;
PCB* t2BottomProgram = NULL;
PCB* t3BottomProgram = NULL;
//PCB* PROCESS0PTR = (PCB*) startPCBAddr; Used for debugging in checkpoint 4
FD_Member* fileArray = NULL;
//Check Available Processes
//6 because we can only have a maximum of 6 processes at once
int processArray[6];
// Fish array to keep track of video memory
int fish[3] = {0, 0, 0};
//Used for filesys.c
int32_t curDirIndex = 0;


//File operations table pointer
void* dirFOTP[4] = {(void*) (dir_read), (void*) (dir_write), (void*) (dir_open), (void*) (dir_close)};
void* rtcFOTP[4] = {(void*) (rtc_read), (void*) (rtc_write), (void*) (rtc_open), (void*) (rtc_close)};
void* fileFOTP[4] = {(void*) (file_read), (void*) (file_write), (void*) (file_open), (void*) (file_close)};
void* terminalFOTP[4] = {(void*) (terminal_read), (void*) (terminal_write), (void*) (terminal_open), (void*) (terminal_close)};

// int32_t execute(const uint8_t* command):
// DESCRIPTION: executes commands
// INPUTS: command - the command to be executed
// OUTPUTS: none
// RETURN VALUE: 0 - If function executed successfully
//               1 - If function did not execute
// SIDE EFFECTS: global variables are updated
int32_t execute(const uint8_t* command)
{
    cli();
    PCB* parentPCBPtr = currPCBPtr;
    //4 to hold 4 characters when checking for "0x7F-E-L-F" in executable check
    uint8_t buf[4];
    //-1 represents no process
    int processNum = -1;
    //128 represents the number of characters we can store
    char executable[128];
    char arguments[128];
    //Clear arrays
    //128 represents size of the buffer
    memset((char*) arguments,0, sizeof(char) * 128);
    memset((char*) executable,0, sizeof(char) * 128);

//Parse Arguments (Check Leading Spaces before exe, Spaces between exe and argument)
    int flag = 0;
    int i = 0;
    int execIndex = 0;
    // Grab the executable
    while(command[i] != '\0')
    {
        // Checks for leading spaces
        if((command[i] == ' ') && (flag == 0)) {i++; continue;}
        // Checks for a space at the end to signify end of executable
        if(flag == 1 && command[i] == ' ')
        {
            executable[execIndex] = '\0';
            flag = 0;
            break;
        }
        // Update flag and copy characters
        flag = 1;
        executable[execIndex++] = command[i++];
    }
    
    //Grab the arguments
   while(command[i] != '\0')
   {
       // Skips leading spaces
       if(command[i] == ' ' && flag == 0) {i++; continue;}
       // Copies the rest after dropping leading spaces
       strcpy((int8_t*)arguments, (int8_t*)&command[i]);
       break;
   }

//Executable Check
    dentry temp;
    if(read_dentry_by_name((uint8_t*) executable, &temp) != 0)
    {
        return -1;
    }
    
    //checks magic numbers that define a ELF
    //0x7F = DEL in ASCII
    //4 represents how many characters we are reading
    read_data(temp.inode_num, 0,(int8_t*) buf, 4);
    if((buf[0] != 0x7F) || (buf[1] != 'E') || (buf[2] != 'L') || (buf[3] != 'F'))
    {
        return -1;
    }
    //6 represents maximum number of processes that can be opened at once
    for(i = 0; i < 6; i++)
    {
        if(processArray[i] == NOTBUSY)
        {
            processNum = i;
            processArray[processNum] = BUSY;
            break;
        }  
    }
    //-1 represents no process
    if(processNum == -1)
    {
        return -1;
    }

//Paging 
    uint32_t page_dir_entry = 32;//128MB / 4MB = 32
    //0x800000 = 8MB
    //0x400000 = 4MB
    uint32_t physAddr = (0x800000 + (processNum * 0x400000));
    //setting page directory bits to approriate values
    page_directory[page_dir_entry].present = 1;
    page_directory[page_dir_entry].read_write = 1;
    page_directory[page_dir_entry].user_supervisor = 1;
    page_directory[page_dir_entry].write_through = 0;
    page_directory[page_dir_entry].cache_disabled = 0;
    page_directory[page_dir_entry].accessed = 0;
    page_directory[page_dir_entry].dirty = 0;
    page_directory[page_dir_entry].page_size = 1;
    page_directory[page_dir_entry].global_page = 0;
    page_directory[page_dir_entry].available = 0;
    page_directory[page_dir_entry].page_table_address = (physAddr >> 12);
    //flush TLB since we updated page directory
    flush_tlb();

//User-Level Program Loader
    //0x0804800 <- Virtual mem address for entire ELF
    //bytes 24-27 (4 bytes) in ELF <- First program instruction location
    uint32_t programStartAddr;

    //24 represents the starting byte for program instruction location
    //4 represents how many bytes we want (24-27)
    read_data(temp.inode_num, 24, (int8_t *) &programStartAddr,4);

    //Copy user program
    //0x0804800 <- Virtual mem address for entire ELF
    read_data(temp.inode_num, 0, (int8_t *)0x8048000, inodePtr[temp.inode_num].length);

//Create PCB
    //Grab processID from Parent (works until you call HALT on first process)
    uint32_t parentProcessID = 0;
    switch (currTask) {
        case terminal1:
            parentProcessID = (t1TopProgram == NULL) ? processNum : t1TopProgram->processID; 
            break;  
        case terminal2:
            parentProcessID = (t2TopProgram == NULL) ? processNum : t2TopProgram->processID; 
            break;  
        case terminal3:
            parentProcessID = (t3TopProgram == NULL) ? processNum : t3TopProgram->processID;
    }
    //Find correct address of PCB given process #  
    //0x800000 = 8MB
    //0x2000 = 8KB blocks 
    currPCBPtr = (PCB*) ((uint32_t) 0x800000 - ((processNum+1) * 0x2000));
    //Clear PCB you're pointing to
    memset(currPCBPtr,0,sizeof(PCB));
    //Set fileArray global variable
    fileArray = &(currPCBPtr->fileArrayPCB[0]);
    //Set stdin(0) and stdout(1) in fileArray
    fileArray[0].fileIOJumpTable = (uint32_t*) &terminalFOTP[0];
    fileArray[0].inode = 0; 
    fileArray[0].filePosition = 0;
    fileArray[0].flags = BUSY;

    fileArray[1].fileIOJumpTable = (uint32_t*) &terminalFOTP[0];
    fileArray[1].inode = 0;
    fileArray[1].filePosition = 0;
    fileArray[1].flags = BUSY;
    //Set Process ID, Parent Process ID, Parent ESP0, Parent SS0
    currPCBPtr->processID = processNum;
    currPCBPtr->parentID = parentProcessID;
    // Parent ESP0
    currPCBPtr->esp0 = tss.esp0; //potentially change
    // Parent SS0
    currPCBPtr->ss0 = tss.ss0;
    // Copies the arguments into the current PCB array entry
    memcpy((char*) currPCBPtr->args, (char *) arguments, sizeof(char) * 128);

    // Saves the top most program address for the terminal
    switch (currTask) {
        case terminal1:  
            // Sets the bottom program to the current program
            if (t1TopProgram == NULL) t1BottomProgram = currPCBPtr;
            // Updates the pointer to point at the new program
            t1TopProgram = currPCBPtr; 
            break;
        case terminal2: 
            // Sets the bottom program to the current program
            if (t2TopProgram == NULL) t2BottomProgram = currPCBPtr;
            // Updates the pointer to point at the new program
            t2TopProgram = currPCBPtr; 
            break;
        case terminal3: 
            // Sets the bottom program to the current program
            if (t3TopProgram == NULL) t3BottomProgram = currPCBPtr;
            // Updates the pointer to point at the new program
            t3TopProgram = currPCBPtr;
    }

    // printf("Execute Process Array: ");
    // for(i = 0; i < 6; i++)
    // {
    //     printf("%d: %d ", i, processArray[i]);
    // } printf("\n");

//Context Switch
    //0x800000 = 8MB
    //0x2000 = 8KB blocks
    //subtracting by four to avoid page fault since stack could start at 8MB
    uint32_t newESP =  0x800000 - (processNum * 0x2000) - 4;
    //save ebp and esp:: yes, needed.
    // Push artifical IRET context(USER_DS, ESP, EFLAGS, CS, EIP) onto the stack
    //return 0 to indicate success
    int32_t returnVal = artificial_IRET(programStartAddr,newESP, (uint32_t) &parentPCBPtr->esp, (uint32_t) &parentPCBPtr->ebp);
    return returnVal;
}

// int32_t halt(uint8_t status):
// DESCRIPTION: halts a process
// INPUTS: status - the value to return to parent process
// OUTPUTS: none
// RETURN VALUE: 0 - to indicate success
// SIDE EFFECTS: global variables are updated
int32_t halt(uint8_t status) {
//WHEN WE HALT FIRST SHELL WE RE-eXECUTE 
//WHEN WE HALT A PROCESS, MAKE SURE TO CHANGE THE currPCBPtr, and fileArray to parent process
    //create local variables
    cli();
    int i;
    PCB* current_PCB = currPCBPtr;
    //0x800000 = 8MB
    //0x2000 = 8KB blocks
    PCB* parent_PCB = (PCB*) (0x800000 - (current_PCB->parentID + 1) * 0x2000);
    processArray[current_PCB->processID] = NOTBUSY;
    //8 represents maximum number of files that can be opened at once in fileArray
    for(i = 0; i < 8; i++)
    {
        current_PCB->fileArrayPCB[i].fileIOJumpTable = NULL;
        if(current_PCB->fileArrayPCB[i].flags == BUSY)
        {
           // current_PCB->fileArrayPCB[i].flags = NOTBUSY;  
			close(i);
        }
    }
    // Saves the top most and bottom most program pointers
    PCB* topMost;
    PCB* bottomMost;
    switch (currTask) {
        case terminal1: 
            topMost = t1TopProgram;
            bottomMost = t1BottomProgram;
            // Sets the fish array to 0 to mark it as unused
            fish[currTask] = 0;
            break;
        case terminal2:
            topMost  = t2TopProgram;
            bottomMost = t2BottomProgram;
            // Sets the fish array to 0 to mark it as unused
            fish[currTask] = 0;
            break;
        case terminal3:
            topMost = t3TopProgram;
            bottomMost = t3BottomProgram;
            // Sets the fish array to 0 to mark it as unused
            fish[currTask] = 0;
    }

    //-1 represents that there are no processes open
    // check if we are halting the last process in
    if (current_PCB->processID == current_PCB->parentID || current_PCB->processID <= 0 || 
        topMost->processID == bottomMost->processID)
    {
        execute((uint8_t *)"shell");
    }
//Restore parent data
    tss.esp0 = current_PCB->esp0;
    tss.ss0 = current_PCB->ss0;
//Restore parent paging
    //128MB / 4MB = 32
    uint32_t page_dir_entry = 32;
    //0x800000 = 8MB
    //0x400000 = 4MB 
    uint32_t physAddr = (0x800000 + (current_PCB->parentID * 0x400000));
    //uint32_t physAddr = (0x800000 + (parent_PCB->processID * 0x400000));
    //setting page directory bits to approriate values
    page_directory[page_dir_entry].present = 1;
    page_directory[page_dir_entry].read_write = 1;
    page_directory[page_dir_entry].user_supervisor = 1;
    page_directory[page_dir_entry].write_through = 0;
    page_directory[page_dir_entry].cache_disabled = 0;
    page_directory[page_dir_entry].accessed = 0;
    page_directory[page_dir_entry].dirty = 0;
    page_directory[page_dir_entry].page_size = 1;
    page_directory[page_dir_entry].global_page = 0;
    page_directory[page_dir_entry].available = 0;
    page_directory[page_dir_entry].page_table_address = (physAddr >> 12);
    // Set video map page directory to not present
    //flush TLB since we updated page directory
    flush_tlb();
    //close any relevant fds / clear fds
    // write parent process info to tss esp0
    //jump to execute return
    //update currPCBPtr global variable to parent PCB
    currPCBPtr = parent_PCB;
    //update fileArray global variable to point to parent PCB's fileArray
    fileArray = &(currPCBPtr->fileArrayPCB[0]);

    // Saves the new top most program address for the terminal
    switch (currTask) {
        case terminal1: t1TopProgram = currPCBPtr; break;
        case terminal2: t2TopProgram = currPCBPtr; break;
        case terminal3: t3TopProgram = currPCBPtr;
    }

    // printf("Halt Process Array: ");
    // for(i = 0; i < 6; i++)
    // {
    //     printf("%d: %d ", i, processArray[i]);
    // } printf("\n");

    //execute assembly linkage
    asm volatile("movl newRetAddr, %eax;"
                "movl %eax, 4(%ebp);");
    //return 0 to indicate success
    if(status == 255) return 256;
    int32_t status32 = status & 0x000000FF;
    return status32;
}

// int32_t open(const uint8_t* filename):
// DESCRIPTION: opens a file and updates fileArray
// INPUTS: filename - name of the file to be opened
// OUTPUTS: none
// RETURN VALUE: fd - If function executed successfully
//               -1 - If function did not execute successfully
// SIDE EFFECTS: global variables are updated
int32_t open(const uint8_t* filename)
{
    //create local variables
    int fd;
    dentry tempDirEntry;
    //Find index that is not in use
    //start at 2 because that is first index
    for(fd = 2; fd < fileArraySize; fd++)
    {
        if(fileArray[fd].flags == NOTBUSY)
        {
            fileArray[fd].flags = BUSY;
            break;
        // -1 means no file slots are open
        }else if(fd == fileArraySize - 1)
        {   
            //No file descriptors available
            return -1;
        }
    }
    //Check if file name exists and sets curDirIndex
    if(read_dentry_by_name(filename, &tempDirEntry)  == -1)
    {
        fileArray[fd].flags = NOTBUSY;
        return -1;
    }
    //0 = rtc, 1 = directory, 2 = regular file
    if(tempDirEntry.filetype == 0)
    {
        //call respective open and update fileArray variables
        rtc_open(filename);
        //rtcFOTP index is 0 because we want starting address of the array
        fileArray[fd].fileIOJumpTable = (uint32_t*) &rtcFOTP[0];
        fileArray[fd].filePosition = 0;
        fileArray[fd].inode = 0;
    }
    if(tempDirEntry.filetype == 1)
    {
        //call respective open and update fileArray variables
        dir_open(filename);
        //dirFOTP index is 0 because we want starting address of the array
        fileArray[fd].fileIOJumpTable = (uint32_t*) &dirFOTP[0];
        fileArray[fd].filePosition = curDirIndex;
        fileArray[fd].inode = 0;
    }
    if(tempDirEntry.filetype == 2)
    {
        //call respective open and update fileArray variables
        file_open(filename);
        //fileFOTP index is 0 because we want starting address of the array
        fileArray[fd].fileIOJumpTable = (uint32_t*) &fileFOTP[0];
        fileArray[fd].filePosition = 0;
        fileArray[fd].inode = tempDirEntry.inode_num;
     }
     //return fd number to indicate success
     return fd;
}

// int32_t close(int32_t fd):
// DESCRIPTION: closes a file and updates fileArray
// INPUTS: fd - file descriptor to be used
// OUTPUTS: none
// RETURN VALUE:  0 - If function executed successfully
//               -1 - If function did not execute successfully
// SIDE EFFECTS: global variables are updated
int32_t close(int32_t fd)
{
    //check if file decriptor number is in bounds
    // must be between 0-7
    if(fd == 1 || fd == 0 || fd > 7 || fd < 0) 
    {
        return -1;
    }
    //Don't close something that isn't opened
    if(fileArray[fd].flags == NOTBUSY) return -1;
    fileArray[fd].flags = NOTBUSY;
    return 0;
}

// int32_t read(int32_t fd, void* buf, int32_t nbytes):
// DESCRIPTION: calls respective read function
// INPUTS: fd - file descriptor to be used
//         buf - buffer to store bytes into
//         nbytes - number of bytes to be read
// OUTPUTS: none
// RETURN VALUE:  (*read_ptr) - return value of correct read function
//               -1 - If function did not execute successfully
// SIDE EFFECTS: global variables are updated
int32_t read(int32_t fd, void* buf, int32_t nbytes)
{
    //check if file decriptor number is in bounds
    // must be between 0-7
    if(fd > 7 || fd < 0)
    {
        return -1;
    }
    if(fd == 1) return -1;
    if(fileArray[fd].flags == NOTBUSY) return -1;
    //set read_ptr to point to correct function pointer
    //fileIOJumpTable index is 0 because that is where the read function is located
    int32_t (*read_ptr)(int32_t fd, void* buf, int32_t nbytes) = (void*) fileArray[fd].fileIOJumpTable[0];
    //call respective read function
    return (*read_ptr)(fd, buf, nbytes);
}

// int32_t write(int32_t fd, const void* buf, int32_t nbytes):
// DESCRIPTION: calls respective write function
// INPUTS: fd - file descriptor to be used
//         buf - buffer to store bytes into
//         nbytes - number of bytes to be read
// OUTPUTS: none
// RETURN VALUE:  (*write_ptr) - return value of correct write function
//               -1 - If function did not execute successfully
// SIDE EFFECTS: global variables are updated
int32_t write(int32_t fd, const void* buf, int32_t nbytes)
{
    //check if file decriptor number is in bounds
    // must be between 0-7
    if(fd > 7 || fd < 0)
    {
        return -1;
    }
    if(fd == 0) return -1;
    if(fileArray[fd].flags == NOTBUSY) return -1;
    //set read_ptr to point to correct function pointer
    //fileIOJumpTable index is 0 because that is where the write function is located
    int32_t (*write_ptr)(int32_t fd, const void* buf, int32_t nbytes) = (void*) fileArray[fd].fileIOJumpTable[1];
    //call respective write function
    return (*write_ptr)( fd, buf, nbytes);
}

// int32_t getargs_handler(uint8_t* buf, int32_t nbytes):
// DESCRIPTION: copies argument from execute into buffer
// INPUTS: nbytes - number of bytes to store
//         buf - buffer to store bytes into
//         nbytes - number of bytes to be read
// OUTPUTS: none
// RETURN VALUE:  0 - If function executed successfully
//               -1 - If function did not execute successfully
// SIDE EFFECTS: None
int32_t getargs_handler(uint8_t* buf, int32_t nbytes) {
    // Checks for valid arguments
    // if buf points to NULL or if the arg length is greater than nbytes, return -1
    if(buf == NULL || strlen(currPCBPtr->args) > nbytes || strlen(currPCBPtr->args) == 0)
    {
        // Returns -1 on failure
        return -1;
    }
    // Copies the string into the user level buffer
    //args index is 0 because we want start address of args buffer
    strcpy((int8_t *)buf, (int8_t *)&(currPCBPtr->args[0]));
    // Returns 0 on success
    return 0;
}

// int32_t vidmap_handler(uint8_t** screen_start):
// DESCRIPTION: Sets up the video memory page
// INPUTS: screen_start - pointer to the address the video map should start at
// OUTPUTS: none
// RETURN VALUE:  0 - If function executed successfully
//               -1 - If function did not execute successfully
// SIDE EFFECTS: None
int32_t vidmap_handler(uint8_t** screen_start) {
    // Check if arguments are valid (Null, or inside the kernel space)
    // Checks if screen_start is located within program image
    // 0x8000000 = 128MB
    // 0x08448000 = 132MB
    // Between 128MB and 132MB because thats where the program image is located in
    if(screen_start == NULL || screen_start < (uint8_t **) 0x8000000 || screen_start >= (uint8_t **) 0x08448000)
    {
        // return -1 screen_start is out of bounds
        return -1;
    }
    // Sets the fish array to 1 to mark it used
    fish[currTask] = 1;
    // 132MB / 4MB = 33 (WILL CHANGE FOR SCHEDULING)
    uint32_t page_dir_entry = 33;
    // set physAddr equal to video memory location
    uint32_t physAddr = VIDEO;
    // setting page directory bits to approriate values
    page_directory[page_dir_entry].present = 1; // set to 1 to make block present
    page_directory[page_dir_entry].read_write = 1; // set to 1 for read/write access
    page_directory[page_dir_entry].user_supervisor = 1; // set to 1 for user privelege
    page_directory[page_dir_entry].write_through = 0;
    page_directory[page_dir_entry].cache_disabled = 0;
    page_directory[page_dir_entry].accessed = 0;
    page_directory[page_dir_entry].dirty = 0;
    page_directory[page_dir_entry].page_size = 0; //set to 0 for 4KB blocks for video memory
    page_directory[page_dir_entry].global_page = 0;
    page_directory[page_dir_entry].available = 0;
    page_directory[page_dir_entry].page_table_address = ((uint32_t)(&vidMapPageTable[0]) >> 12);
    // set video map page table
    vidMapPageTable[0].present = 1; // set to 1 for page to be present
    vidMapPageTable[0].read_write = 1; // set to 1 for read/write access
    vidMapPageTable[0].user_supervisor = 1; // // set to 1 for user privelege
    vidMapPageTable[0].write_through = 0;
    vidMapPageTable[0].cache_disabled = 0;
    vidMapPageTable[0].accessed = 0;
    vidMapPageTable[0].dirty = 0;
    vidMapPageTable[0].page_table_attribute_index = 0;
    vidMapPageTable[0].global_page = 0;
    vidMapPageTable[0].available = 0;
    vidMapPageTable[0].page_base_address = (physAddr)>>12; // set page base address to video address
    //flush TLB since we updated page directory
    flush_tlb();
    // set screen start to point to first page table
    // 0x400000 = 4MB because we have 4MB pages
    (*screen_start) = ((uint8_t *)(page_dir_entry * 0x400000)); //132MB (Uses first page table)
    // Return 0 for success
    return 0;
}

// int32_t set_handler_handler(int32_t signum, void* handler):
// DESCRIPTION: Handles signals
// INPUTS: signum - The signal to handler
//         handler - The handler to use
// OUTPUTS: none
// RETURN VALUE:  0 - If function executed successfully
//               -1 - If function did not execute successfully
// SIDE EFFECTS: None
int32_t set_handler_handler(int32_t signum, void* handler) {
    // Return -1 since signals aren't implemented
    return -1;
}

// int32_t sigreturn_handler(void):
// DESCRIPTION: Handles signals
// INPUTS: none
// OUTPUTS: none
// RETURN VALUE:  0 - If function executed successfully
//               -1 - If function did not execute successfully
// SIDE EFFECTS: None
int32_t sigreturn_handler(void) {
    // Return -1 since signals aren't implemented
    return -1;
}

// int32_t copyProgram(const uint8_t* command, int processNum)
// {
//   PCB* parentPCBPtr = currPCBPtr;
//     //4 to hold 4 characters when checking for "0x7F-E-L-F" in executable check
//     uint8_t buf[4];
//     //-1 represents no process
//    // int processNum = -1;
//     //128 represents the number of characters we can store
//     char executable[128];
//     char arguments[128];
//     //Clear arrays
//     //128 represents size of the buffer
//     memset((char*) arguments,0, sizeof(char) * 128);
//     memset((char*) executable,0, sizeof(char) * 128);

// //Parse Arguments (Check Leading Spaces before exe, Spaces between exe and argument)
//     int flag = 0;
//     int i = 0;
//     int execIndex = 0;
//     // Grab the executable
//     while(command[i] != '\0')
//     {
//         // Checks for leading spaces
//         if((command[i] == ' ') && (flag == 0)) {i++; continue;}
//         // Checks for a space at the end to signify end of executable
//         if(flag == 1 && command[i] == ' ')
//         {
//             executable[execIndex] = '\0';
//             flag = 0;
//             break;
//         }
//         // Update flag and copy characters
//         flag = 1;
//         executable[execIndex++] = command[i++];
//     }
    
//     //Grab the arguments
//    while(command[i] != '\0')
//    {
//        // Skips leading spaces
//        if(command[i] == ' ' && flag == 0) {i++; continue;}
//        // Copies the rest after dropping leading spaces
//        strcpy((int8_t*)arguments, (int8_t*)&command[i]);
//        break;
//    }

// //Executable Check
//     dentry temp;
//     if(read_dentry_by_name((uint8_t*) executable, &temp) != 0)
//     {
//         return -1;
//     }
    
//     //checks magic numbers that define a ELF
//     //0x7F = DEL in ASCII
//     //4 represents how many characters we are reading
//     read_data(temp.inode_num, 0,(int8_t*) buf, 4);
//     if((buf[0] != 0x7F) || (buf[1] != 'E') || (buf[2] != 'L') || (buf[3] != 'F'))
//     {
//         return -1;
//     }
// /*
//     //6 represents maximum number of processes that can be opened at once
//     for(i = 0; i < 6; i++)
//     {
//         if(processArray[i] == NOTBUSY)
//         {
//             processNum = i;
//             processArray[processNum] = BUSY;
//             break;
//         }
//     }
//     //-1 represents no process
//     if(processNum == -1)
//     {
//         return -1;
//     }
// */

// //Paging 
//     uint32_t page_dir_entry = 32;//128MB / 4MB = 32
//     //0x800000 = 8MB
//     //0x400000 = 4MB
//     uint32_t physAddr = (0x800000 + (processNum * 0x400000));
//     //setting page directory bits to approriate values
//     page_directory[page_dir_entry].present = 1;
//     page_directory[page_dir_entry].read_write = 1;
//     page_directory[page_dir_entry].user_supervisor = 1;
//     page_directory[page_dir_entry].write_through = 0;
//     page_directory[page_dir_entry].cache_disabled = 0;
//     page_directory[page_dir_entry].accessed = 0;
//     page_directory[page_dir_entry].dirty = 0;
//     page_directory[page_dir_entry].page_size = 1;
//     page_directory[page_dir_entry].global_page = 0;
//     page_directory[page_dir_entry].available = 0;
//     page_directory[page_dir_entry].page_table_address = (physAddr >> 12);
//     //flush TLB since we updated page directory
//     flush_tlb();

// //User-Level Program Loader
//     //0x0804800 <- Virtual mem address for entire ELF
//     //bytes 24-27 (4 bytes) in ELF <- First program instruction location
//     uint32_t programStartAddr;

//     //24 represents the starting byte for program instruction location
//     //4 represents how many bytes we want (24-27)
//     read_data(temp.inode_num, 24, (int8_t *) &programStartAddr,4);

//     //Copy user program
//     //0x0804800 <- Virtual mem address for entire ELF
//     read_data(temp.inode_num, 0, (int8_t *)0x8048000, inodePtr[temp.inode_num].length);

// //Create PCB
//     //Grab processID from Parent (works until you call HALT on first process)

//     uint32_t parentProcessID = (processNum) ? currPCBPtr->processID : 0;
//     //Find correct address of PCB given process #  
//     //0x800000 = 8MB
//     //0x2000 = 8KB blocks 
//     currPCBPtr = (PCB*) ((uint32_t) 0x800000 - ((processNum+1) * 0x2000));
//     //Clear PCB you're pointing to
//     memset(currPCBPtr,0,sizeof(PCB));
//     //Set fileArray global variable
//     fileArray = &(currPCBPtr->fileArrayPCB[0]);
//     //Set stdin(0) and stdout(1) in fileArray
//     fileArray[0].fileIOJumpTable = (uint32_t*) &terminalFOTP[0];
//     fileArray[0].inode = 0; 
//     fileArray[0].filePosition = 0;
//     fileArray[0].flags = BUSY;

//     fileArray[1].fileIOJumpTable = (uint32_t*) &terminalFOTP[0];
//     fileArray[1].inode = 0;
//     fileArray[1].filePosition = 0;
//     fileArray[1].flags = BUSY;
//     //Set Process ID, Parent Process ID, Parent ESP0, Parent SS0
//     currPCBPtr->processID = processNum;
//     currPCBPtr->parentID = parentProcessID;
//     // Parent ESP0
//     currPCBPtr->esp0 = tss.esp0; //potentially change
//     // Parent SS0
//     currPCBPtr->ss0 = tss.ss0;
//     // Copies the arguments into the current PCB array entry
//     memcpy((char*) currPCBPtr->args, (char *) arguments, sizeof(char) * 128);

// }
