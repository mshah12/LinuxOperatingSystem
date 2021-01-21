/* terminal_driver.h - Defines what is used in interactions with the terminal
 * vim:ts=4 noexpandtab
 */

#ifndef _TERMINAL_DRIVER_H
#define _TERMINAL_DRIVER_H

// Defines the number of columns
#define NUM_COLS    80
#define NUM_ROWS    25

// Variables for the keyboard buffer of size 128 characters
extern char kb_buf[128];
extern int kb_buf_index, kb_start_index, overflow;

// Global functions used for the terminal driver
int32_t terminal_open(void);
int32_t terminal_close(void);
int32_t terminal_read(int32_t fd, char* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const char* buf, int32_t nbytes);

#endif /* _TERMINAL_DRIVER_H */
