# interrupt.S - Set up x86 segment descriptors, descriptor tables
# vim:ts=4 noexpandtab

#define ASM     1
#include "interrupt.h"
//#include "terminals.h"

.text
.globl exception_handler_prep, system_call_prep, other_interrupt_handler_prep
.globl rtc_interrupt_handler_prep, kb_interrupt_handler_prep, pit_handler_prep
.globl int_num_0, int_num_15

# Variable for the exception number
int_num: .long -1

.align 4
# Exception to update interrupt number
int_num_0:
cli
movl $0, int_num
jmp exception_handler_prep
.align 4
int_num_1:
cli
movl $1, int_num
jmp exception_handler_prep
.align 4
int_num_2:
movl $2, int_num
jmp exception_handler_prep
.align 4
int_num_3:
cli
movl $3, int_num
jmp exception_handler_prep
.align 4
int_num_4:
cli
movl $4, int_num
jmp exception_handler_prep
.align 4
int_num_5:
cli
movl $5, int_num
jmp exception_handler_prep
.align 4
int_num_6:
cli
movl $6, int_num
jmp exception_handler_prep
.align 4
int_num_7:
cli
movl $7, int_num
jmp exception_handler_prep
int_num_8:
cli
movl $8, int_num
jmp exception_handler_prep
.align 4
int_num_9:
cli
movl $9, int_num
jmp exception_handler_prep
int_num_10:
cli
movl $10, int_num
jmp exception_handler_prep
.align 4
int_num_11:
cli
movl $11, int_num
jmp exception_handler_prep
.align 4
int_num_12:
cli
movl $12, int_num
jmp exception_handler_prep
.align 4
int_num_13:
cli
movl $13, int_num
jmp exception_handler_prep
.align 4
int_num_14:
cli
movl $14, int_num
jmp exception_handler_prep
.align 4
int_num_15:
cli
movl $15, int_num
jmp exception_handler_prep
.align 4
int_num_16:
cli
movl $16, int_num
jmp exception_handler_prep
.align 4
int_num_17:
cli
movl $17, int_num
jmp exception_handler_prep
.align 4
int_num_18:
cli
movl $18, int_num
jmp exception_handler_prep
.align 4
int_num_19:
cli
movl $19, int_num
jmp exception_handler_prep

.align 16
# Setup for the exception handler
exception_handler_prep:
# Pushes all registers
pushal
pushfl
pushl int_num

# Calls the exception handler
call exception_handler

# Pops and returns
addl $4, %esp
popfl
# Moves return value to the spot of eax
movl %eax, 28(%esp)
popal
sti
iret


# Jump table for system call
.align 16
system_call_table:
.long halt, execute, read, write, open, close, getargs_handler, vidmap_handler, set_handler_handler, sigreturn_handler
/*
// System call macro from ece391syscall.S
#define DO_CALL(name,number)   \
.GLOBL name                   ;\
name:   PUSHL	%EBX          ;\
	MOVL	$number,%EAX  ;\
	MOVL	8(%ESP),%EBX  ;\
	MOVL	12(%ESP),%ECX ;\
	MOVL	16(%ESP),%EDX ;\
	PUSHL %EDX			;\
	PUSHL %ECX			;\
	PUSHL %EBX			;\
	INT	$0x80         ;\
	POPL %EBX			;\
	RET
*/
/* the system call library wrappers */
/*DO_CALL(setup_halt, 1)
DO_CALL(setup_execute, 2)
DO_CALL(setup_read, 3)
DO_CALL(setup_write, 4)
DO_CALL(setup_open, 5)
DO_CALL(setup_close, 6)
DO_CALL(setup_getargs, 7)
DO_CALL(setup_vidmap, 8)
DO_CALL(setup_set_handler, 9)
DO_CALL(setup_sigreturn, 10)
*/
.align 16
# Setup for the system call handler
system_call_prep:
# Saves registers
pushal

# Finds the system call number
addl $-1, %eax
# Checks for valid inputs
cmpl $0, %eax
jl invalid_entry
cmpl $9, %eax
jg invalid_entry
#imul $4, %eax 

# Push arguments for the function 
pushl %edx
pushl %ecx
pushl %ebx

# Jumps to the system call (should be jmp)
call *system_call_table(,%eax,4)

# Pop arguments
popl %ebx
popl %ecx
popl %edx
# Moves return value to the spot of eax
movl %eax, 28(%esp)
popal
iret

# Returns if invalid entries
invalid_entry:
movl $-1, %eax
movl %eax, 28(%esp)
popal
iret

.align 16
# Setup for the RTC interrupt handler
rtc_interrupt_handler_prep:
# Pushes all registers
pushal
pushfl

# Calls the RTC interrupt handler
call rtc_interrupt_handler

# Pops and returns
popfl
popal
iret

.align 16
# Setup for the keyboard interrupt handler
kb_interrupt_handler_prep:
# Pushes all registers
pushal
pushfl

# Calls the keyboard interrupt handler
call kb_interrupt_handler
# Add a label so we can jump to this part 

# Pops and returns
popfl
popal
iret

.align 16
# Setup for the PIT interrupt handler
pit_handler_prep:
# Pushes all registers
pushal
pushfl

# Calls the PIT interrupt handler
call pit_handler

# Pops and returns
popfl
popal
iret

.align 16
# Setup for the other interrupt handler
other_interrupt_handler_prep:
# Pushes all registers
pushal
pushfl

# Calls the other interrupt handler
call other_interrupt_handler

# Pops and returns
popfl
popal
iret
