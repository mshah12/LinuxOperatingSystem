#define ASM     1
//#include "sys_call_struct.h"
#include "x86_desc.h"

.data
# Constants
ARG1 = 8
ARG2 = 12
ARG3 = 16
ARG4 = 20
ebpStructOffset = 12
espStructOffset = 16
.globl flush_tlb
.globl artificial_IRET
.globl currPCBPtr,newRetAddr

newRetAddr: .long 0x0

.text 

flush_tlb:
// Callee setup
	pushl %ebp // push old base pointer
	movl %esp, %ebp // make EBP point to old base pointer
//--------------------
// reloads CR3 register to flush TLB
    movl %cr3, %eax
    movl %eax, %cr3
//--------------------    
    leave
    RET

artificial_IRET:
// Callee setup
	pushl %ebp // push old base pointer
	movl %esp, %ebp // make EBP point to old base pointer
// Callee save
    pushl %EBX
    pushl %ESI
    pushl %EDI
// Grab argument(s)
    movl ARG1(%EBP), %EBX // has program start address
    movl ARG2(%EBP), %ESI // new ESP0
    cli;
// Write new process' info to TSS
    movl $tss, %EDI 
    movl %ESI ,4(%EDI) // esp0
    movl  $KERNEL_DS, 8(%EDI) // ss0, Kernel_DS
// Push IRET context(USER_DS, ESP, EFLAGS, CS, EIP) onto the stack
    movl $USER_DS, %EAX
    mov %AX, %DS //USER_DS
    mov %AX, %FS //USER_DS
    mov %AX, %GS //USER_DS
    mov %AX, %ES //USER_DS
    movl ARG3(%EBP), %EDI //parent ESP
    movl %ESP, (%EDI) // both need to stop being global
    movl ARG4(%EBP), %EDI //parent EBP
    movl %EBP, (%EDI) //
    pushl $USER_DS //USER_DS
    pushl $0x83FFFFC //New process user esp (bottom of user page) 128MB+4MB
    pushfl
    popl %eax
    orl $0x200, %eax // OR by 0x200 to reenable interrupts
    pushl %eax
    pushl $USER_CS //USER_CS
    pushl %EBX //Needs to be program start address
    movl $returnAddr, newRetAddr
// Calls IRET after pushing IRET context  
    IRET
returnAddr: //needs argument now
    movl currPCBPtr, %EDI 
    movl ebpStructOffset(%EDI), %EBP 
    movl espStructOffset(%EDI), %ESP
    popl %EDI
    popl %ESI
    popl %EBX
    sti
    leave
    RET
