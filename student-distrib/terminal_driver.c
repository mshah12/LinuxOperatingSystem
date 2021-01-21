/* terminal_driver.c - Functions to interact with the terminal
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include "terminal_driver.h"
#include "terminals.h"

// Global variables
int screen_x;
int screen_y;
int kb_buf_index;
int kb_start_index;
int overflow;

/* Function that intializes the terminal driver
 * Inputs: none
 * Outputs: none
 */
int32_t terminal_open(void) {
    // Returns 0 since that is the file descriptor
    return 0;
}

/* Function that closes the terminal driver
 * Inputs: none
 * Outputs: none
 */
int32_t terminal_close(void) {
    // Returns -1 since you can never close the terminal
    return -1;
}

/* Function that reads a line from the keyboard
 * Inputs: buf - buffer to write values to from the keyboard
 *          fd - file descripptor(should be 0 for terminal driver)
 *          nbytes - not needed
 * Outputs: none
 */
int32_t terminal_read(int32_t fd, char* buf, int32_t nbytes) {
    // Initializes variables
    uint32_t i = 0, size = nbytes;

    // Checks if argument is valid or if the right fd is given
    if (buf == NULL || fd != 0) return -1;

    
    // Waits until it sees a \n character
    while ((kb_buf[(kb_buf_index - 1)] != '\n') || currTask != currTerminal) {}
    // Finds the size of the kb buffer
    size = kb_buf_index;

    // Checks if there is an overflow
    if ((kb_buf_index == kb_start_index) && (overflow != 0)) {
        size = 128;
        overflow = 0;
    }
    // Copys the number of values from the start to the end with wrapping until it reaches a \n
    for (i = 0; i < size; i++) {
        // Copies the ith value from the start into the ith index in the input buffer
        buf[i] = kb_buf[(kb_start_index + i) % 128] ;

        // Checks for \0 character in the ith index from the start
        if (kb_buf[(kb_start_index + i) % 128] == '\n') {
            // Change the NULL character with a new line
            buf[i] = '\n';

            // When found update the indices
            kb_start_index = 0;
            kb_buf_index = 0;

            // Checks if there is two '\n' back to back
            if (i > 0) {
                if (buf[i] == buf[i - 1]) return i;
            }

            // Returns the count of iterations 
            return i + 1;
        }
    }

    // Returns 0 if a new line was not found
    
    return 0;
}


/* Function that write nbytes from a buffer
 * Inputs: buf - buffer to write from, nbytes - number of bytes to write 
 *         fd - file descriptor(should be 1 for the terminal)
 * Outputs: none
 */
int32_t terminal_write(int32_t fd, const char* buf, int32_t nbytes) {
    // Initializes variables
    int i = 0;

    // Checks if arguments are valid(fd is 1, buf is not NULL, and nbytes is in range)
    if (buf == NULL || nbytes < 0 || fd != 1) return -1;

    // Loops through and prints nbytes of buffer
    for (; i < nbytes; i++) {
	//If you want to skip more than just NULL uncomment line below
		//if(buf[i] < 32) continue;
		if(buf[i] == '\0') continue;
        // Adds a new line at the end of the terminal
        if (screen_x == NUM_COLS - 1) {
		   // Prints the character
            putc(buf[i]);

            // Checks if the previous or next input is \n if so don't add \n
            if (i > 0) {
                if (buf[i - 1] == '\n') continue;
            }
            if (buf[i] == '\n') continue;
            if (i < 127) {
                if (buf[i + 1] == '\n') continue;
            }

            // Adds the new line if it is needed
            putc('\n'); //REMOVED CAUSE I DON"T THINK IT"S NECESSARY - ISH
            continue;
        }

        // Prints the character
        putc(buf[i]);
    }

    // Returns the number of bytes written on success
    return nbytes;
}
