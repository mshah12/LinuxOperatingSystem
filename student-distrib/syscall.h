#ifndef SYSCALL_H
#define SYSCALL_H

#include "lib.h"
#include "types.h"
#include "rtc.h"
#include "filesys.h"

#define NOTBUSY 0
#define BUSY  1

//Process PCB start addresses
#define process0PCBAddr 0x7FE000
#define process1PCBAddr 0x7FC000
#define process2PCBAddr 0x7FA000
#define process3PCBAddr 0x7F8000
#define process4PCBAddr 0x7F6000
#define process5PCBAddr 0x7F4000
//Process 0 start address basically
#define startPCBAddr process0PCBAddr

typedef struct FD_Member{
    //File operations jump table 
    uint32_t* fileIOJumpTable;
    //inode number for the particular file
    uint32_t inode;
    //keeps track where the user is currently reading from in the file
    //If a directory, keeps directory index
    uint32_t filePosition;
    //Flags, marks this file as "inUse"
    uint32_t flags;
}__attribute__ ((packed)) FD_Member;

//Store arguments in PCB
typedef struct PCB{
    // Process ID
    uint32_t processID;
    // Parent ID
    uint32_t parentID;
    // Parent ESP0 (TSS)
    uint32_t esp0;
    // PCB's EBP and ESP DON"T ADD ANYTHING ABOVE
    uint32_t ebp;
    uint32_t esp;
    // Current PCB's EBP and ESP
    uint32_t currEBP;
    uint32_t currESP;
    // Parent SS0 (TSS)
    uint16_t ss0;
    uint16_t ss0_pad;
    // File Descriptor Array
    FD_Member fileArrayPCB[8];
    // Argument Array 
    char args[128];
}__attribute__ ((packed)) PCB;

//8MB Process 0: (kernel stack, PCB(FD Array, other stuff))
//execute will make this point to FD array for the current process 
extern FD_Member* fileArray;

//Current Directory Index from read_dentry_by_name
extern int32_t curDirIndex;

//Allows terminal switching to access process array, file array, and currPCBPtr
extern int processArray[6];
extern PCB* currPCBPtr;
FD_Member* fileArray;

// Array to keep track of which terminals are using video memory
extern int fish[3];

/* Functions defined in sys_call.S */
//flush tlb assembly function
extern void flush_tlb();
// Function to push IRET context and call IRET
extern int32_t artificial_IRET(uint32_t, uint32_t,uint32_t, uint32_t);
//extern void after_IRET();
extern uint32_t newRetAddr;
//successful calls should return 0, else return -1

int32_t halt(uint8_t status);

int32_t execute(const uint8_t* command);

int32_t copyProgram(const uint8_t* command, int processNum);

int32_t read(int32_t fd, void* buf, int32_t nbytes);

int32_t write(int32_t fd, const void* buf, int32_t nbytes);

int32_t open(const uint8_t* filename);

int32_t close(int32_t fd);

int32_t getargs_handler(uint8_t* buf, int32_t nbytes);

int32_t vidmap_handler(uint8_t** screen_start);

int32_t set_handler_handler(int32_t signum, void* handler);

int32_t sigreturn_handler(void);

#endif
