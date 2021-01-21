/* interrupt_handler.c - the C part of the interrupt handler
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include "interrupt.h"
#include "i8259.h"
#include "keyboard.h"
#include "rtc.h"
#include "terminal_driver.h"
#include "syscall.h"
#include "terminals.h"
#include "paging.h"

#define IRQ8 8
#define VIDEO 0xB8000

// Function to deal with exceptions
// Input: num - The number of the exception
// Output: none
int32_t exception_handler(int num) {
    //printf("The exception with the number %d has occurred\n", num);
    halt(255);
    return -1;
}

// Function to deal with rtc interrupts
// Input: none
// Output: none
void rtc_interrupt_handler() {

	interruptcount++;
	//test_interrupts();
	outb(regC, CMOSRW70);	// select register C
	inb(REGNUM71);		// just throw away contents

	send_eoi(IRQ8); //send eoi so next tick can come in, and other interrupts to occur
	int_seen = 1; //signal that interrupt has been received and acted upon, for rtc_read

}

/* Keyboard global variables */
// 128 since the max characters the buffer should hold is 128
char t1Keyboard_Buf[128];
char t2Keyboard_Buf[128];
char t3Keyboard_Buf[128];
// Initializes keyboard buffer values
char kb_buf[128];
// set values to default 0
int kb_buf_index = 0, kb_start_index = 0, overflow = 0, clear_flag = 0;
int t1_kb_index = 0, t2_kb_index = 0, t3_kb_index = 0;
// Stores whether special keys are being pressed
uint8_t shift = 0, ctrl = 0, alt = 0, tab = 0, caps = 0;
// Stores the screen cursor locations
int screen_x, screen_y;
int t1_screen_x, t2_screen_x, t3_screen_x;
int t1_screen_y, t2_screen_y, t3_screen_y;

// Function to deal with keyboard interrupts
// Input: none
// Output: none
void kb_interrupt_handler() {
    // Initializes variables
    unsigned char key_input;

    // Gets keyboard key
    key_input = inb(KEYBOARD_DATA);

    /* Checks for special keys and updates state variables */
    // Backspace pressed(0x0E)
    if (key_input == 0x0E) {
        /* Updates the screen cursors and prints ' ' */
        // Gets the correct cursor coordinates
        switch (currTerminal) {
        case terminal1:
            screen_x = t1_screen_x;
            screen_y = t1_screen_y;
            break;
        case terminal2:
            screen_x = t2_screen_x;
            screen_y = t2_screen_y;
            break;
        case terminal3:
            screen_x = t3_screen_x;
            screen_y = t3_screen_y;
            break;
        }

        // Checks if the duffer index is at the start_index
        if (kb_buf_index == kb_start_index) {
            // Sends EOI and returns
            send_eoi(1);
            return;
        }
        // Checks if the cursor is at x = 0
        else if (screen_x == 0) {
            // Sends the cursor to the prev line
            screen_x = NUM_COLS - 1;
            screen_y = (screen_y - 1) % NUM_ROWS;
            switch (currTerminal) {
            case terminal1:
                t1_screen_x = screen_x;
                t1_screen_y = screen_y;
                break;
            case terminal2:
                t2_screen_x = screen_x;
                t2_screen_y = screen_y;
                break;
            case terminal3:
                t3_screen_x = screen_x;
                t3_screen_y = screen_y;
                break;
            }

            // Gets rid of the new line
            // modulo 128 sunce there are 128 indices
            kb_buf_index = (kb_buf_index - 1) % 128;

            // Prints ' '
            putc_keyboard(' ');

            // Update cursor
            screen_x = NUM_COLS - 1;
        }
        // Every other case
        else {
            // Updates cursor and prints space to remove character
            screen_x--;
            switch (currTerminal) {
            case terminal1:
                t1_screen_x = screen_x;
                t1_screen_y = screen_y;
                break;
            case terminal2:
                t2_screen_x = screen_x;
                t2_screen_y = screen_y;
                break;
            case terminal3:
                t3_screen_x = screen_x;
                t3_screen_y = screen_y;
                break;
            }
            putc_keyboard(' ');

            // Updates cursor again
            screen_x--;
        }

        // Removes one from the buffer
        // modulo 128 sunce there are 128 indices
        kb_buf_index = (kb_buf_index - 1) % 128;

        // Checks if previous value is a clear screen (127)
        if (kb_buf[kb_buf_index] == 127) clear_flag = 0;

        // Saves and updates cursor
        switch (currTerminal) {
        case terminal1:
            t1_screen_x = screen_x;
            t1_screen_y = screen_y;
            break;
        case terminal2:
            t2_screen_x = screen_x;
            t2_screen_y = screen_y;
            break;
        case terminal3:
            t3_screen_x = screen_x;
            t3_screen_y = screen_y;
            break;
        }
        update_cursor(screen_x, screen_y);

        // Sends EOI and returns
        send_eoi(1);
        return;
    }
    // Tab pressed(0xF0) or released(0x8F)
    else if (key_input == 0xF0 || key_input == 0x8F) {
        // Updates the tab value to 1 when pressed; 0 otherwise
        tab = (key_input == 0xF0) ? 1 : 0;

        /* Tab counts as four spaces and adds to the buffer */
        // Initializes variables
        int i = 0;
        // Tries to add four spaces into the buffer
        for (; i < 4; i++) {
            // modulo 128 sunce there are 128 indices
            if (((kb_buf_index - kb_start_index) % 128) < 127) {
                // Adds a space to the buffer
                kb_buf[kb_buf_index] = ' ';
                // Increments with cyclical wrapping
                // modulo 128 sunce there are 128 indices
                kb_buf_index = (kb_buf_index + 1) % 128;
                // Prints the new line character
                putc_keyboard(' ');
            }
            else
                // Signals that the buffer index will be the same as the start after enter
                overflow = 1;
        }

        // Sends EOI and returns
        send_eoi(1);
        return;
    }
    // Enter pressed(0x1C)
    else if (key_input == 0x1C) {
        // Stores the null character to signify enter has been pressed
        kb_buf[kb_buf_index] = '\n';
        // Increments
        kb_buf_index = (kb_buf_index + 1);
        // Prints the new line character if there is nothing on the line
        //if ((screen_x != 0)) 
        putc_keyboard('\n');

        // Clears the screen if need be
            if (clear_flag) {
                // Clears the screen and updates cursor values
                clear();
                switch (currTerminal) {
                case terminal1:
                    screen_x = t1_screen_x;
                    screen_y = t1_screen_y;
                    break;
                case terminal2:
                    screen_x = t2_screen_x;
                    screen_y = t2_screen_y;
                    break;
                case terminal3:
                    screen_x = t3_screen_x;
                    screen_y = t3_screen_y;
                    break;
                }
                screen_x = 0;
                screen_y = 0;
                clear_flag = 0;

                switch (currTerminal) {
                case terminal1:
                    t1_screen_x = screen_x;
                    t1_screen_y = screen_y;
                    break;
                case terminal2:
                    t2_screen_x = screen_x;
                    t2_screen_y = screen_y;
                    break;
                case terminal3:
                    t3_screen_x = screen_x;
                    t3_screen_y = screen_y;
                    break;
                }

                // Resets the buffer
                kb_start_index = 0;
                // kb_buf[kb_buf_index] = '\0';
                // kb_buf_index = 0;

                // Updates the cursor and returns
                update_cursor(screen_x, screen_y);
            }

        // Sends EOI and returns
        send_eoi(1);
        return;
    }
    // Control pressed(0x1D) or released(0x9D)
    else if (key_input == 0x1D || key_input == 0x9D) {
        // Updates the control value if control was pressed or released
        ctrl = (key_input == 0x1D) ? 1 : 0;

        // Sends EOI and returns
        send_eoi(1);
        return;
    }
    // Left or Right Shift pressed(0x2A, 0x36) or released(0xAA, 0xB6) respectively
    else if (key_input == 0x2A || key_input == 0x36 || key_input == 0xAA || key_input == 0xB6) {
        // Updates shift depending on if shift was pressed or released
        shift = (key_input == 0x2A || key_input == 0x36) ? 1 : 0;

        // Sends EOI and returns
        send_eoi(1);
        return;
    }
    // Left alt pressed(0x38) or released(0xB8)
    else if (key_input == 0x38 || key_input == 0xB8) {
        // Updates the alt value if alt was pressed or released
        alt = (key_input == 0x38) ? 1 : 0;

        // Sends EOI and returns
        send_eoi(1);
        return;
    }
    // Caps Lock pressed(0x3A)
    else if (key_input == 0x3A) {
        // Toggle caps lock variable
        caps = (caps) ? 0 : 1;

        // Sends EOI and returns
        send_eoi(1);
        return;
    }
    // Checks if you need to switch terminals
    if (alt) {
        uint32_t destPtr;
        //Check if terminal 1
        if(currTerminal != terminal1 && key_input == 0x3B)//0x1A)
        {
            // Sends EOI and halt
            send_eoi(1);
			rtc_freq_set(t1RTC);
            // Saves previous keyboard buffer
            if (currTerminal == terminal3) memcpy((void*) t3Keyboard_Buf, (void*) kb_buf, kb_buf_index);
            else memcpy((void*) t2Keyboard_Buf, (void*) kb_buf, kb_buf_index);
            // Switch input buffer and index
            memcpy((void*) kb_buf, (void*) t1Keyboard_Buf, t1_kb_index);
            if (currTerminal == terminal2) t2_kb_index = kb_buf_index;
            else t3_kb_index = kb_buf_index;
            kb_buf_index = t1_kb_index;
            
            // Save current terminal screen
            destPtr = (currTerminal == terminal2) ? t2VideoVirtualAddr : t3VideoVirtualAddr;
            
            // Updates video_mem to the correct value
            //184 represents terminal 1 page table
            uint32_t terminal = page_table[184].page_base_address;
            // right shift since we only want the MSB for address
            page_table[184].page_base_address = (VIDEO)>>12;
            flush_tlb();
            // 4096 since 4KB blocks
            memcpy((void*) destPtr, (void*) VIDEO, 4096);
            
            // Restore terminal 1's screen to video memory
            // 4096 since 4MB blocks
            memcpy((void*) VIDEO, (void*) t1VideoVirtualAddr, 4096);
            // right shift since we only want the MSB for address
            page_table[184].page_base_address = (terminal)>>12; 
            flush_tlb();
            // Restores screen cursors
            screen_x = t1_screen_x;
            screen_y = t1_screen_y;
            update_cursor(screen_x, screen_y);

            // Update current terminal
            currTerminal = terminal1;
            
            // Returns afterwards
            return;
        }
        // Check if terminal 2 
        if(currTerminal != terminal2 && key_input == 0x3C)//0x1B)
        {
            // Sends EOI and halt
            send_eoi(1);
			rtc_freq_set(t2RTC);
            // Saves previous keyboard buffer
            if (currTerminal == terminal1) memcpy((void*) t1Keyboard_Buf, (void*) kb_buf, kb_buf_index);
            else memcpy((void*) t3Keyboard_Buf, (void*) kb_buf, kb_buf_index);
            // Switch input buffer and index
            memcpy((void*) kb_buf, (void*) t2Keyboard_Buf, t2_kb_index);
            if (currTerminal == terminal1) t1_kb_index = kb_buf_index;
            else t3_kb_index = kb_buf_index;
            kb_buf_index = t2_kb_index;
            
            // Save current terminal screen
            destPtr = (currTerminal == terminal1) ? t1VideoVirtualAddr : t3VideoVirtualAddr;

            // Updates video_mem to the correct value
            //184 represents terminal 2 page table
            uint32_t terminal = page_table[184].page_base_address;
            // right shift since we only want the MSB for address
            page_table[184].page_base_address = (VIDEO)>>12;
            flush_tlb();
            // 4096 since 4KB blocks
            memcpy((void*) destPtr, (void*) VIDEO, 4096);
            
            // Restore terminal 2's screen to video memory
            // 4096 since 4KB blocks
            memcpy((void*) VIDEO, (void*) t2VideoVirtualAddr, 4096);
            // right shift since we only want the MSB for address
            page_table[184].page_base_address = (terminal)>>12; 
            flush_tlb();
            // Restores screen cursors
            screen_x = t2_screen_x;
            screen_y = t2_screen_y;
            update_cursor(screen_x, screen_y);

            // Update current terminal
            currTerminal = terminal2;
            
            // Returns afterwards
            return;
        }
        //Check if terminal 3
        if(currTerminal != terminal3 && key_input == 0x3D)//0x2B)
        {
            // Sends EOI and halt
            send_eoi(1);
			rtc_freq_set(t3RTC);
            // Saves previous keyboard buffer
            if (currTerminal == terminal1) memcpy((void*) t1Keyboard_Buf, (void*) kb_buf, kb_buf_index);
            else memcpy((void*) t2Keyboard_Buf, (void*) kb_buf, kb_buf_index);
            // Switch input buffer and index
            memcpy((void*) kb_buf, (void*) t3Keyboard_Buf, t3_kb_index);
            if (currTerminal == terminal1) t1_kb_index = kb_buf_index;
            else t2_kb_index = kb_buf_index;
            kb_buf_index = t3_kb_index;
            
            // Save current terminal screen
            destPtr = (currTerminal == terminal1) ? t1VideoVirtualAddr : t2VideoVirtualAddr;

            // Updates video_mem to the correct value
            //184 represents terminal 3 page table
            uint32_t terminal = page_table[184].page_base_address;
            // right shift since we only want the MSB for address
            page_table[184].page_base_address = (VIDEO)>>12;
            flush_tlb();
            // 4096 since 4KB blocks
            memcpy((void*) destPtr, (void*) VIDEO, 4096);
            
            // Restore terminal 3's screen to video memory
            // 4096 since 4KB blocks
            memcpy((void*) VIDEO, (void*) t3VideoVirtualAddr, 4096);
            // right shift since we only want the MSB for address
            page_table[184].page_base_address = (terminal)>>12; 
            flush_tlb();

            // Restores screen cursors
            screen_x = t3_screen_x;
            screen_y = t3_screen_y;
            update_cursor(screen_x, screen_y);
            
            // Update current terminal
            currTerminal = terminal3;
           
            // Returns afterwards
            return;
        }
        // Sends EOI and halt
        send_eoi(1);
        // Returns afterwards
        return;
    }

    /* Handles the overflow conditions */
    // Checks if the difference between the start index and the current index is 127 values
    if (((kb_buf_index - kb_start_index) % 128) == 127) {
        // Signals that the buffer index will be the same as the start after enter
        overflow = 1;

        // If the difference is 127 skip adding anything and returns
        send_eoi(1);
        return;
    }

    // Gets key mapping of the key
    key_input = key_mapping(key_input, shift, caps);

    // Checks if clear screen(CTRL + L) was pressed
    if (ctrl && (key_input == 'L' || key_input == 'l')) {
        // Sets the clear flag
        clear_flag = 1;

        // Prints ' '
        putc_keyboard(' ');

        // Sends the clear screen value(127)
        kb_buf[kb_buf_index] = 127;
        // Increments with cyclical wrapping
        // modulo 128 since there is a max of 128 characters
        kb_buf_index = (kb_buf_index + 1) % 128;

        // Returns afterwards
        send_eoi(1);
        return;
    }

    // Checks if interrupt program(CTRL + C) was pressed
    if (ctrl && (key_input == 'C' || key_input == 'c')) {
        // Sends EOI and halt
        send_eoi(1);
        putc_keyboard('\n');
        // Changes currTask to currTerminal to halt the correct program
        currTask = currTerminal;
        // Updates current pointer so halt can halt the right program
        switch(currTerminal) {
            case terminal1:
                currPCBPtr = t1TopProgram;
                break;
            case terminal2:
                currPCBPtr = t2TopProgram;
                break;
            case terminal3:
                currPCBPtr = t3TopProgram;
        }
        halt(0);

        // Returns afterwards
        return;
    }

    // Checks if key_input is NULL
    if (key_input != NULL) {
        // Updates buffer values
        kb_buf[kb_buf_index] = key_input;
        // Increments with cyclical wrapping
        // modulo 128 since there are a max of 128 characters
        kb_buf_index = (kb_buf_index + 1) % 128;
        // Prints value of the typed character and a new line if it needs it
        switch (currTerminal) {
            case terminal1:
                screen_x = t1_screen_x;
                break;
            case terminal2:
                screen_x = t2_screen_x;
                break;
            case terminal3:
                screen_x = t3_screen_x;
                break;
        }
        if (screen_x == (NUM_COLS - 1)) {putc_keyboard(key_input); putc_keyboard('\n');}
        else putc_keyboard(key_input);
    }

    // Sends EOI to the PIC
    send_eoi(1);
}

// Function to deal with keyboard interrupts
// Input: none
// Output: none
void other_interrupt_handler() {
    printf("Other interrupt has occured\n");
    while (1) {}
}
