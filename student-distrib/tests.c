#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "terminal_driver.h"
#include "filesys.h"
#include "rtc.h"
#include "syscall.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* GDT Test - Example
 * 
 * Asserts that GDT does not bootloop
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: GDT definition
 * Files: x86_desc.h/S, boot.S
 */
int gdt_test(){
	TEST_HEADER;

	// Initializes variables
	int result = PASS;

	// Returns the result
	return result;
}

/* IDT Test 1 - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test_1(){
	TEST_HEADER;
	
	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* IDT Test 2 - Example
 * 
 * Asserts int 1
 * Inputs: None
 * Outputs: Exception has/has not occurred
 * Side Effects: None
 * Coverage: IDT definition, IDT handler
 * Files: x86_desc.h/S
 */
int idt_test_2(){
	TEST_HEADER;

	int result = PASS;
	
	asm volatile("int $0x9");

	return result;
}

/* IDT Test 3 - Example
 * 
 * Asserts int x3
 * Inputs: None
 * Outputs: Exception has/has not occurred
 * Side Effects: None
 * Coverage: IDT definition, IDT handler
 * Files: x86_desc.h/S
 */
int idt_test_3(){
	TEST_HEADER;
	
	int result = PASS;

	asm volatile("int $0x3");

	return result;
}

/* System call test - Example
 * 
 * Asserts int 0x80
 * Inputs: None
 * Outputs: Exception has/has not occurred
 * Side Effects: None
 * Coverage: IDT definition, IDT handler
 * Files: x86_desc.h/S
 */
int system_call_test(){
	TEST_HEADER;
	
	int result = PASS;

	asm volatile("int $0x80");

	return result;
}

int page_fault_test(){
	TEST_HEADER;
	
	//int result = PASS;
	int* tempx = (int*)0;
	
	int tempy = *tempx;
	//Never reach here
	return tempy;
	
}
int page_video_sucess(){
	TEST_HEADER;
	
	int result = PASS;
	int* tempx = (int*)0xB8000;
	
	int tempy = *tempx;
	printf("no exception! Value: %d", tempy);
	//Never reach here
	return result;
}
int page_kernel_success(){
	TEST_HEADER;
	
	int result = PASS;
	int* tempx = (int*)0x400000;
	
	int tempy = *tempx;
	printf("no exception! Value: %d", tempy);
	//Never reach here
	return result;
	
}
int page_fault_test_end(){
	TEST_HEADER;
	
	//int result = PASS;
	int* tempx = (int*)0xFFFFFFFF;
	
	int tempy = *tempx;
	//Never reach here
	return tempy;
	
}

/* Checkpoint 2 tests */
/* Terminal Driver Test
 * Loops through and continuously calls terminal_read and terminal_write
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Terminal Drivers and Keyboard Buffer
 * Files: interrupt_handler.c, terminal_driver.h/S
 */
int terminal_driver_test() {
	TEST_HEADER;

	// Initializes variables
	char buf[128];
	int nbytes = 0, fd = 0;

	// Runs the terminal driver functions
	fd = terminal_open();
	while (1) {
		nbytes = terminal_read(fd, buf, 0);
		terminal_write(1, buf, nbytes);
	}
	terminal_close();

	return FAIL;
}

/* Terminal Write Overflow Test
 * Tries to get terminal_write to write more/less than the buffer size
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Terminal Drivers
 * Files: terminal_driver.h/S
 */
int terminal_write_overflow_test() {
	TEST_HEADER;

	// Initializes buf
	char buf[128];
	int test_more, test_less;

	// Tries to tell terminal_write to use more than 128 bytes
	test_more = terminal_write(1, buf, 129);  

	// Tries to tell terminal_write to use less than 0 bytes
	test_less = terminal_write(1, buf, -1);

	// Returns whether the tests passed or failed
	if (test_more == -1 && test_less == -1) return PASS;
	return FAIL;
}

int file_exe_test(){
	
	int8_t words[128];
	//words[10] = '\0';
	file_open((uint8_t*)"ls");
	while(0 == file_read(2, words,128))
	{
	terminal_write(1, words, 128);
	}	
	file_close(2);
	putc('\n');
	return PASS;
}
int file_long_test(){
	
	int8_t words[128];
	//words[10] = '\0';
	file_open((uint8_t*)"verylargetextwithverylongname.tx");
	while(0 == file_read(2, words,128))
	{
	terminal_write(1, words, 128);
	}	
	file_close(2);
	putc('\n');
	return PASS;
}
int file_short_test(){
	
	int8_t words[128];
	//words[10] = '\0';
	int32_t fd = open((uint8_t*)"frame0.txt");
	while(0 == read(fd, words,128))
	{
	terminal_write(1, words, 128);
	}	
	file_close(fd);
	putc('\n');
	return PASS;
}
int dir_output_test(){
	
	int8_t words[32];
	//words[10] = '\0';
	int32_t fd = open((uint8_t*)".");
	while(-1 != read(fd, words,32))
	{
	terminal_write(1, words, 32);
	putc('\n');
	}	
	close(fd);
	return PASS;
}

int rtc_driver_valid_freq2(){
	TEST_HEADER;
	int32_t freqtoset;
	int32_t readstat;
	int32_t wstat;
	//argument init for functions
	
	int32_t buf[1]; //one entry in my buffer for rtc testing
	int32_t nbytes = 4; 
	int32_t fd = open((uint8_t*)"rtc");
	freqtoset =2;
	buf[0] = freqtoset;
	
	wstat = write(fd, buf, nbytes);
	while(1){
		readstat = read(fd, buf, nbytes);
		if(!readstat){
			putc('1');
		}
	}
	close(fd);
	
	return PASS;
}

/*
function: rtc_driver_valid_freq64()
input: none
output: pass or fail status
description: test  if frequency can be succesfully set to 64 Hz , prints 1 to screen
*/
int rtc_driver_valid_freq64(){
	TEST_HEADER;
	int32_t freqtoset;
	int32_t readstat;
	int32_t wstat;
	//argument init for functions
	
	int32_t buf[1]; //one entry in my buffer for rtc testing
	int32_t nbytes = 4; 
	int32_t fd = 0;
	uint8_t filename = 0xAA; 
	const uint8_t* fn = &filename;
	fd = rtc_open(fn);
	freqtoset =64;
	buf[0] = freqtoset;
	
	wstat = rtc_write(fd, buf, nbytes);
	while(1){
		readstat = rtc_read(fd, buf, nbytes);
		if(!readstat){
			putc('1');
		}
	}
	rtc_close(fd);
	
	return PASS;
}

/*
function: rtc_driver_valid_freq256()
input: none
output: pass or fail status
description: test  if frequency can be succesfully set to 256 Hz , prints 1 to screen
*/
int rtc_driver_valid_freq256(){
	TEST_HEADER;
	int32_t freqtoset;
	int32_t readstat;
	int32_t wstat;
	//argument init for functions
	
	int32_t buf[1]; //one entry in my buffer for rtc testing
	int32_t nbytes = 4; 
	int32_t fd = 0;
	uint8_t filename = 0xAA; 
	const uint8_t* fn = &filename;
	fd = rtc_open(fn);
	freqtoset =256;
	buf[0] = freqtoset;
	
	wstat = rtc_write(fd, buf, nbytes);
	while(1){
		readstat = rtc_read(fd, buf, nbytes);
		if(!readstat){
			putc('1');
		}
	}
	rtc_close(fd);
	
	return PASS;
}

/*
function: rtc_driver_valid_freq1024()
input: none
output: pass or fail status
description: test  if frequency can be succesfully set to 1024 Hz , prints 1 to screen
*/
int rtc_driver_valid_freq1024(){
	TEST_HEADER;
	int32_t freqtoset;
	int32_t readstat;
	int32_t wstat;
	//argument init for functions
	
	int32_t buf[1]; //one entry in my buffer for rtc testing
	int32_t nbytes = 4; 
	int32_t fd = 0;
	uint8_t filename = 0xAA; 
	const uint8_t* fn = &filename;
	fd = rtc_open(fn);
	freqtoset =1024;
	buf[0] = freqtoset;
	
	wstat = rtc_write(fd, buf, nbytes);
	while(1){
		readstat = rtc_read(fd, buf, nbytes);
		if(!readstat){
			putc('1');
		}
	}
	rtc_close(fd);
	
	return PASS;
}

/*
function: rtc_driver_open()
input: none
output: pass or fail status
description: test rtc_open() function
*/
int rtc_driver_open(){
	TEST_HEADER;
	int32_t freqtoset;
	int32_t readstat;
	//argument init for functions
	
	int32_t buf[1]; //one entry in my buffer for rtc testing
	int32_t nbytes = 4; 
	int32_t fd = 0;
	uint8_t filename = 0xAA; 
	const uint8_t* fn = &filename;
	fd = rtc_open(fn);
	if(fd == 0){
		printf("rtc open \n");
	}
	freqtoset =256;
	buf[0] = freqtoset;
	
	while(1){
		readstat = rtc_read(fd, buf, nbytes);
		if(!readstat){
			putc('1');
		}
	}
	rtc_close(fd);
	
	return PASS;
}

/*
function: rtc_driver_close()
input: none
output: pass or fail status
description: test rtc_close()
*/
int rtc_driver_close(){
	TEST_HEADER;
	int32_t freqtoset;
	int32_t wstat;
	//argument init for functions
	
	int32_t buf[1]; //one entry in my buffer for rtc testing
	int32_t fd = 0;
	uint8_t filename = 0xAA; 
	const uint8_t* fn = &filename;
	fd = rtc_open(fn);
	freqtoset =256;
	buf[0] = freqtoset;
	
	wstat = rtc_close(fd);
	if(wstat == 0){
		printf("close successful \n");
	}
	
	return PASS;
}

/*
function: rtc_driver_invalid_freq()
input: none
output: pass or fail status
description: test  if invalid test frequencies will be thrown out
*/
int rtc_driver_invalid_freq(){
	TEST_HEADER;
	int32_t freqtoset;
	int32_t readstat;
	int32_t wstat;
	//argument init for functions
	
	int32_t buf[1]; //one entry in my buffer for rtc testing
	int32_t nbytes = 4; 
	int32_t fd = 0;
	uint8_t filename = 0xAA; 
	const uint8_t* fn = &filename;
	fd = rtc_open(fn);
	freqtoset =12;
	buf[0] = freqtoset;
	
	wstat = rtc_write(fd, buf, nbytes);
	if(wstat == -1){
	printf("Invalid: did not set to frequency of %d \n", freqtoset);
	}
	while(1){
		readstat = rtc_read(fd, buf, nbytes);
		if(!readstat){
			putc('1');
		}
	}
	rtc_close(fd);
	
	return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	// Checkpoint 1 Tests
	//TEST_OUTPUT("gdt_test", gdt_test());
	//TEST_OUTPUT("idt_test_1", idt_test_1());
	//TEST_OUTPUT("idt_test_2", idt_test_2());
	//TEST_OUTPUT("idt_test_3", idt_test_3());
	//TEST_OUTPUT("system_call_test", system_call_test());
	//TEST_OUTPUT("Address 0x0 Page Fault", page_fault_test());
	//TEST_OUTPUT("Acessing Video Memory", page_video_sucess());
	//TEST_OUTPUT("Acessing Kernel Memory", page_kernel_success());
	//TEST_OUTPUT("Last Addr. Page Fault", page_fault_test_end());

	// Checkpoint 2 Tests
	//TEST_OUTPUT("dir_output_test", dir_output_test());
	//TEST_OUTPUT("file_output_test", file_exe_test());
	//TEST_OUTPUT("file_output_test", file_long_test());
	//TEST_OUTPUT("file_output_test", file_short_test());

	//TEST_OUTPUT("terminal_write_inputs", terminal_write_overflow_test());
	//TEST_OUTPUT("terminal_driver_test", terminal_driver_test());
	
	
	//TEST_OUTPUT("rtc test, test_interrupts", test_interrupts());
	//TEST_OUTPUT("rtc valid interrupts freq 2", rtc_driver_valid_freq2());
	//TEST_OUTPUT("rtc valid interrupts freq 64", rtc_driver_valid_freq64());
	//TEST_OUTPUT("rtc valid interrupts freq 256", rtc_driver_valid_freq256());
	//TEST_OUTPUT("rtc valid interrupts freq 1024", rtc_driver_valid_freq1024());
	//TEST_OUTPUT("rtc open", rtc_driver_open());
	//TEST_OUTPUT("rtc close", rtc_driver_close());
	//TEST_OUTPUT("rtc invalid interrupts", rtc_driver_invalid_freq());
}
