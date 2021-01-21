#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "types.h"
#include "rtc.h"
#include "i8259.h"
#include "terminals.h"

unsigned int interruptcount;//for virtualization, unimplemented as of yet
unsigned int int_seen = 0; // for rtc_read, signal to see if interrupt handler has run or not, will be turned on in interrupt handler


void rtc_init(){
	enable_irq(IRQ8);
	interruptcount = 0;
	outb(regC, CMOSRW70);	// select register C
	inb(REGNUM71);		// just throw away contents
	start_rtc_periodic_interrupt();
	
}
/*
function: start_rtc_periodic_interrupt
input: none
output: none
description: connects rtc to irq8 in idt, at default fequency of 1024 Hz
*/
void start_rtc_periodic_interrupt(){
	//code based on osdev example 
	uint32_t saveprevval;
	char turnon6;
	cli(); 							 //disable all incoming interrupts
	outb(regB, CMOSRW70);  				//select register B of CMOS and disable NMI for it
	saveprevval = inb(REGNUM71);		//save value stored currently in regB
	outb(regB, CMOSRW70);				//reselect regB, as reading from regB resets the selecting index
	turnon6 = saveprevval | 0x40;  //calculate the value that will turn bit 6 of the register B content on while preserving the rest of the info: 0x40 is a bitmask
	outb(turnon6, REGNUM71);			//write the above calculated value to regB
	sti();
}

/*
function: rtc_open
input: const uint8_t* filename, ptr to file to open
output: success or fail status
description: opens communiation with rtc
*/
int32_t rtc_open(const uint8_t* filename){
	
	rtc_freq_set(2); //set frq to 2
	return 0;
}

/*
function: rtc_close
input: int32_t fd, file descriptor of file needed to  be closed
output: success or failure of closure status
description: closes rtc file
*/
int32_t rtc_close(int32_t fd){
	return 0;
	//just returns 0 for right now, based on discussion
}

/*
function: rtc_read
input: file descriptor, buffer, number of bytes to be read
output: number of bytes read 
description: reads frq of operation of rtc from buffer (? NEEDS REVIEW ?)
*/
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
	//int32_t cont;
	int_seen = 0;
	//cont = 0;
	while (1){
		//block, will be turned to 1 in interrupt handler
		//cont++;
		//printf("I'm stuck in the read while loop %d \n", cont);
		if(int_seen == 1){
			return 0;
		}
	}
	return 0;
}

/*
function: rtc_write
input: file descriptor, buffer, number of bytes to be read
output: number of bytes written
description: sets operating frequency of rtc
*/
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
	int32_t setfreq;
	if(nbytes != 4){ //byte limit is 4 bytes, no less no more can be written
		return -1; //cannot write
	}
	if(buf == NULL){
		return -1;
		//rtc_freq_set(2); //if there's nothing in the buffer, set frq to the default value
		//return nbytes;
	}
	setfreq = *((int32_t*) buf);
	if(setfreq <MINFREQ || setfreq > MAXFREQ){ //2 is min freq, 1024 is max freq
		return -1; //cannot write, as setfreq is invalid
	}
	if (!powtwo(setfreq)){
		return -1; //cannot write if frq is not a power of two 
	}
	    // Restoring the current PCB pointer, physical address to the program, and the fileArray 
    switch (currTask) {
            // starting address will be at 8MB which is why we add 0x800000 to the address
            // multiplying processID by 0x400000 because we have 4MB blocks
            case terminal1: 
			t1RTC = setfreq;
                break;
            case terminal2:
			t2RTC = setfreq;
                break;
            case terminal3:
			t3RTC = setfreq;
    }
	rtc_freq_set(setfreq);
	return nbytes;
}

//helper function to set frequency
/*
function: rtc_freq_set
input: desired frequency
output: none
description: helper function to set frequency, used by rtc_write and rtc_read
*/

void rtc_freq_set(int32_t frq){
	//find rate first, given desired freq;
	int32_t temp;
	char prev, rate, setvalue;
	rate = 1;
	while(1){
		temp = MAXRATE >> (rate-1);
		if(frq == temp){
			break; //break out of the while loop if you find the rate that produces the frequency we want
		}
		else{
			rate++;
		}
	}
	//set the frequency now, using found rate
	cli();
	outb(regA, CMOSRW70);
	prev = inb(REGNUM71);
	outb(regA, CMOSRW70);
	setvalue = (prev & 0xF0) | rate; //generate value to send to the registerA in CMOS to set freq. this line is done to load relevant bits of rate while preserving previously existing values in CMOS, 0xf0 is a bitmask
	outb(setvalue, REGNUM71);
	
	//reenable interrupts, in case this is the probelm
	outb(regC, CMOSRW70);	// select register C
	inb(REGNUM71);		// just throw away contents
	sti();
	
	return;
}

/*
function: powtwo
input: desired frequency
output: 1 or 0 (indicating true or false)
description: helper function to check if the desired frequency is a power of two number
source: https://www.geeksforgeeks.org/program-to-find-whether-a-no-is-power-of-two/
*/
int32_t powtwo(int32_t frq){
	if(frq == 0){
		return 0;
	}
	while(frq != 1){
		if(frq%2 != 0){
			return 0; //odd number
		}
		frq = frq/2;
	}
	return 1; // is a power of 2
	
}

