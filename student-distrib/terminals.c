#include "terminals.h"
#include "paging.h"
#include "i8259.h"
#include "interrupt.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
//0xB8000 represents video memory location
#define VIDEO 0xB8000
// 0 to represent IRQ line 0
#define IRQ0 0

// Global Variables
uint32_t currTerminal = terminal1;
uint32_t currTask = terminal1;
//set default shellNumber to 0
int shellNumber = 0;

int32_t t1RTC = 2;int32_t t2RTC = 2;int32_t t3RTC = 2;
// void pit_init():
// DESCRIPTION: Initalizes the PIT for interrupt handling
// INPUTS: none
// OUTPUTS: none
// RETURN VALUE: none
// SIDE EFFECTS: manipulates PIT IRQ line 0 to allow interrupts
void pit_init() {
    // Changes frequency of the PIT between 10ms to 50ms
    // set to 100Hz since that is the recommended setting for a real kernel
    change_frequency(100);
    // Enables interrupts
    // input is 0 because that is the IRQ line we are manipulating
    enable_irq(0);
}

// void change_frequency(int hz):
// DESCRIPTION: Changes the frequency of the PIT
// INPUTS: hz - The frequency we want to set it to
// OUTPUTS: none
// RETURN VALUE: none
// SIDE EFFECTS: manipulates PIT freqency
// From OSDever
void change_frequency(int hz)
{
    // 1193180 represents the timer's input clock of 1.19MHz
    /* Calculate our divisor */
    int divisor = (1193180 / hz);       
    //0x36 represents PIT square wave mode 3
    /* Set our command byte 0x36 */
    outb(0x36, 0x43);             
    //0xFF represents frequency mask
    //0x40 represents PIT channel 0
    /* Set low byte of divisor */
    // sets lower 8 bits of the 16-bit data register
    outb(divisor & 0xFF, 0x40);   
    /* Set high byte of divisor */
    // since the data register is a 16-bit value, we shift right by 8 to set the high 8 bits
    outb(divisor >> 8, 0x40);     
}

// void pit_handler():
// DESCRIPTION: Handles PIT interrupts and performs context switching
// INPUTS: none
// OUTPUTS: none
// RETURN VALUE: none
// SIDE EFFECTS: none
void pit_handler() {
    // Initialize variables used in switching processes
    int ebp, esp; 
    int physAddr;
    // CLI to prevent interrupts
    cli();
    // Sends EOI
    send_eoi(IRQ0);
    // Saves current EBP and ESP
    asm volatile (  "mov %%ebp, %0;\n"
                    "mov %%esp, %1;\n"
                    : "=r"(ebp), "=r"(esp));
    //checks current task and determines if the terminal's EBP and ESP values need to be updated
    // follows round-robin style
    if (currTask == terminal1) {
        t1TopProgram->currEBP = ebp;
        t1TopProgram->currESP = esp;
    } 
    else if (currTask == terminal2) {
        t2TopProgram->currEBP = ebp;
        t2TopProgram->currESP = esp;
    }
    else {
        t3TopProgram->currEBP = ebp;
        t3TopProgram->currESP = esp;
    }

    // Starts shell for the other terminals
    if (shellNumber < 3) {
        // Updates the current task and shell number
        shellNumber++;
        currTask = (shellNumber == 1) ? 0 : (currTask + 1) % 3;
        // Reenables interrupts
        sti();
        // Executes a new instance of shell
        execute((uint8_t *) "shell"); 
    }

    // Increment current terminal
    // modulo 3 since we can only have 3 terminals open
    currTask = (currTask + 1) % 3;
	// Restoring the current PCB pointer, physical address to the program, and the fileArray 
    switch (currTask) {
            // starting address will be at 8MB which is why we add 0x800000 to the address
            // multiplying processID by 0x400000 because we have 4MB blocks
            case terminal1: 
                physAddr = (0x800000 + (t1TopProgram->processID * 0x400000));
                currPCBPtr = t1TopProgram;
                break;
            case terminal2:
                currPCBPtr = t2TopProgram;
                physAddr = (0x800000 + (t2TopProgram->processID * 0x400000));
                break;
            case terminal3:
                currPCBPtr = t3TopProgram;
                physAddr = (0x800000 + (t3TopProgram->processID * 0x400000));
    }
    //indexed at 0 since we want starting address of the array
    fileArray = &(currPCBPtr->fileArrayPCB[0]);
    
    /* Restore parent data */ 
    // Restoring TSS
    //0x800000 to represent 8MB starting location
    // multipying by 0x2000 because thats the PCB block width
    // subtracting by 4 to ensure there is no page fault
    uint32_t newESP =  0x800000 - (currPCBPtr->processID * 0x2000) - 4;
    tss.esp0 = newESP;//currPCBPtr->currESP;
    tss.ss0 = currPCBPtr->ss0; 
    //setting page directory bits to approriate values
    uint32_t page_dir_entry = 32; //128MB / 4MB = 32
    // right shifted by 12 because the most significant 20 bits hold the address
    page_directory[page_dir_entry].page_table_address = (physAddr >> 12);

    // Checks to see if the current terminal is using video memory
    if (fish[currTask] == 1 && currTask == currTerminal) 
        // right shifted by 12 because the most significant 20 bits hold the address
        vidMapPageTable[0].page_base_address = (VIDEO)>>12;
    else {
        switch (currTask) {
            // right shifted by 12 because the most significant 20 bits hold the address
            case terminal1: vidMapPageTable[0].page_base_address = (t1VideoMemPhysAddr)>>12; break;
            case terminal2: vidMapPageTable[0].page_base_address = (t2VideoMemPhysAddr)>>12; break;
            case terminal3: vidMapPageTable[0].page_base_address = (t3VideoMemPhysAddr)>>12; break;
        }
    }

    // Restoring ebp and esp 
    asm volatile (  "mov %0, %%ebp;\n"
                    "mov %1, %%esp;\n"
                    : :"r"(currPCBPtr->currEBP), "r"(currPCBPtr->currESP));

    // Flushes the TLBs
    flush_tlb();
    // Returns afterwards and reenables interrupts
    sti();
    return;
}

// void terminalPagingInit():
// DESCRIPTION: Initalizes the video memory for the terminals
// INPUTS: none
// OUTPUTS: none
// RETURN VALUE: none
// SIDE EFFECTS: updates page table values
void terminalPagingInit()
{

//Terminal 1 video memory paging
    //Set the video memory page table
    //page table entry 185 because of terminal 1
    //present = 1 due to video memory being loaded into physical memory
    //global_page = 1 to prevent flushing
    //page_base_address = 0xB9000 as that is where video memory starts
    page_table[185].present = 1;
    page_table[185].read_write = 1; 
    page_table[185].user_supervisor = 0;
    page_table[185].write_through = 0;
    page_table[185].cache_disabled = 0;
    page_table[185].accessed = 0;
    page_table[185].dirty = 0;
    page_table[185].page_table_attribute_index = 0;
    page_table[185].global_page = 0;
    page_table[185].available = 0;
    page_table[185].page_base_address = (t1VideoMemPhysAddr)>>12; 
//Terminal 2 video memory paging
    //Set the video memory page table
    //page table entry 186 because of terminal 2
    //present = 1 due to video memory being loaded into physical memory
    //global_page = 1 to prevent flushing
    //page_base_address = 0xBA000 as that is where video memory starts
    page_table[186].present = 1;
    page_table[186].read_write = 1; 
    page_table[186].user_supervisor = 0;
    page_table[186].write_through = 0;
    page_table[186].cache_disabled = 0;
    page_table[186].accessed = 0;
    page_table[186].dirty = 0;
    page_table[186].page_table_attribute_index = 0;
    page_table[186].global_page = 0;
    page_table[186].available = 0;
    page_table[186].page_base_address = (t2VideoMemPhysAddr)>>12; 
//Terminal 3 video memory paging
    //Set the video memory page table
    //page table entry 187 because of terminal 3
    //present = 1 due to video memory being loaded into physical memory
    //global_page = 1 to prevent flushing
    //page_base_address = 0xBB000 as that is where video memory starts
    page_table[187].present = 1;
    page_table[187].read_write = 1; 
    page_table[187].user_supervisor = 0;
    page_table[187].write_through = 0;
    page_table[187].cache_disabled = 0;
    page_table[187].accessed = 0;
    page_table[187].dirty = 0;
    page_table[187].page_table_attribute_index = 0;
    page_table[187].global_page = 0;
    page_table[187].available = 0;
    page_table[187].page_base_address = (t3VideoMemPhysAddr)>>12; 
    //flush TLB since we updated page directory
    flush_tlb();
}
