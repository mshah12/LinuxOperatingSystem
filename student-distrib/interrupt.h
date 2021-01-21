/* interrupt.h - Defines for various interrupt handlers
 * vim:ts=4 noexpandtab
 */

#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#ifndef ASM

// Variables for interrupt handling
extern uint32_t exception_handler_prep, system_call_prep, other_interrupt_handler_prep;
extern uint32_t rtc_interrupt_handler_prep, kb_interrupt_handler_prep, pit_handler_prep;
extern uint32_t int_num_0, int_num_15;
extern int t1_screen_x, t2_screen_x, t3_screen_x;
extern int t1_screen_y, t2_screen_y, t3_screen_y;

// Set up system call functions
// Copied from ece391syscall.h
extern int32_t setup_halt (uint8_t status);
extern int32_t setup_execute (const uint8_t* command);
extern int32_t setup_read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t setup_write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t setup_open (const uint8_t* filename);
extern int32_t setup_close (int32_t fd);
extern int32_t setup_getargs (uint8_t* buf, int32_t nbytes);
extern int32_t setup_vidmap (uint8_t** screen_start);
extern int32_t setup_set_handler (int32_t signum, void* handler);
extern int32_t setup_sigreturn (void);


#endif /* ASM */

#endif /* _INTERRUPT_H */
