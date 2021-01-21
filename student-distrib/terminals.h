#ifndef TERMINALS_H
#define TERMINALS_H

#include "types.h"
#include "syscall.h"

#define terminal1 0
#define terminal2 1
#define terminal3 2
//starting video memory address for terminal 1
#define t1VideoMemPhysAddr 0xB9000
//starting video memory address for terminal 2
#define t2VideoMemPhysAddr 0xBA000
//starting video memory address for terminal 3
#define t3VideoMemPhysAddr 0xBB000
//starting video memory address for terminal 1
#define t1VideoVirtualAddr 0xB9000
//starting video memory address for terminal 2
#define t2VideoVirtualAddr 0xBA000
//starting video memory address for terminal 3
#define t3VideoVirtualAddr 0xBB000

// Initializes the PIT
void pit_init();
//Allocates Video Memory for Each Terminal
void terminalPagingInit();
// PIT handler
void pit_handler();
// PIT change frequency
void change_frequency(int hz);
//Current Terminal Window that we are looking at
extern uint32_t currTerminal;
extern uint32_t currTask;
extern PCB* t1TopProgram;
extern PCB* t2TopProgram;
extern PCB* t3TopProgram;
extern int32_t t1RTC;
extern int32_t t2RTC;
extern int32_t t3RTC;

#endif
